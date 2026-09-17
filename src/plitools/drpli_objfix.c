/*
 * Fix PL/I-86 OBJ files for compatibility with LINK86.
 *
 * The PL/I-86 compiler V1.0 produces OBJ files that have two issues:
 *
 * 1. SEGDEF records have segment length = 0 as placeholders.
 *    LINK86 requires the correct segment sizes. This tool scans
 *    LEDATA records to compute actual sizes and patches SEGDEFs.
 *
 * 2. FIXUPP records use thread subrecords (F=1 / T=1 references).
 *    LINK86 v1.4 does not handle these correctly. This tool expands
 *    all thread-based fixup references to explicit (F=0 / T=0) form,
 *    eliminating thread subrecords entirely.
 *
 * The output file is also padded to a 128-byte boundary as required
 * by the CP/M-86 file system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define REC_SEGDEF  0x98
#define REC_LEDATA  0xA0
#define REC_FIXUPP  0x9C
#define REC_MODEND  0x8A
#define REC_EOF     0x1A

/* ---- growable byte buffer ---- */

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
} ByteBuf;

static void buf_init(ByteBuf *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_reserve(ByteBuf *b, size_t extra) {
    if (b->len + extra <= b->cap)
        return;
    size_t newcap = b->cap ? b->cap * 2 : 256;
    while (newcap < b->len + extra)
        newcap *= 2;
    b->data = realloc(b->data, newcap);
    if (!b->data) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    b->cap = newcap;
}

static void buf_append(ByteBuf *b, const uint8_t *bytes, size_t n) {
    buf_reserve(b, n);
    memcpy(b->data + b->len, bytes, n);
    b->len += n;
}

static void buf_push(ByteBuf *b, uint8_t byte) {
    buf_append(b, &byte, 1);
}

static void buf_free(ByteBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* ---- OMF record list ---- */

typedef struct {
    int type;
    uint8_t *payload;
    size_t len;
} Record;

typedef struct {
    Record *items;
    size_t count;
    size_t cap;
} RecordList;

static void rl_init(RecordList *rl) {
    rl->items = NULL;
    rl->count = 0;
    rl->cap = 0;
}

static void rl_push(RecordList *rl, int type, const uint8_t *payload, size_t len) {
    if (rl->count == rl->cap) {
        rl->cap = rl->cap ? rl->cap * 2 : 16;
        rl->items = realloc(rl->items, rl->cap * sizeof(Record));
        if (!rl->items) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    Record *r = &rl->items[rl->count++];
    r->type = type;
    r->len = len;
    if (len) {
        r->payload = malloc(len);
        memcpy(r->payload, payload, len);
    } else {
        r->payload = NULL;
    }
}

static void rl_free(RecordList *rl) {
    for (size_t i = 0; i < rl->count; i++)
        free(rl->items[i].payload);
    free(rl->items);
    rl->items = NULL;
    rl->count = rl->cap = 0;
}

/* Parse OBJ data into a list of (record_type, payload) records.
   Stops at MODEND or EOF marker (0x1A). */
static RecordList parse_records(const uint8_t *data, size_t len) {
    RecordList records;
    rl_init(&records);
    size_t pos = 0;
    while (pos < len) {
        if (pos + 3 > len)
            break;
        int rt = data[pos];
        if (rt == REC_EOF)
            break;
        unsigned rlen = data[pos + 1] | (data[pos + 2] << 8);
        size_t end = pos + 3 + rlen;
        if (end > len)
            break;
        /* payload excludes the trailing checksum byte */
        size_t plen = rlen > 0 ? (size_t)(rlen - 1) : 0;
        rl_push(&records, rt, data + pos + 3, plen);
        if (rt == REC_MODEND)
            break;
        pos = end;
    }
    return records;
}

/* Write a single OMF record with correct length and checksum. */
static void write_record(ByteBuf *buf, int rtype, const uint8_t *payload, size_t len) {
    unsigned rlen = (unsigned)(len + 1); /* +1 for checksum byte */
    uint8_t header[3] = { (uint8_t)rtype, (uint8_t)(rlen & 0xFF), (uint8_t)((rlen >> 8) & 0xFF) };
    unsigned chk = 0;
    for (int i = 0; i < 3; i++)
        chk = (chk + header[i]) & 0xFF;
    for (size_t i = 0; i < len; i++)
        chk = (chk + payload[i]) & 0xFF;
    buf_append(buf, header, 3);
    buf_append(buf, payload, len);
    buf_push(buf, (uint8_t)((-(int)chk) & 0xFF));
}

/* Scan LEDATA records to compute actual segment sizes.
   seg_sizes[i] == -1 means "no data seen for segment i". */
static void compute_seg_sizes(const RecordList *records, long seg_sizes[256]) {
    for (int i = 0; i < 256; i++)
        seg_sizes[i] = -1;
    for (size_t i = 0; i < records->count; i++) {
        const Record *r = &records->items[i];
        if (r->type != REC_LEDATA)
            continue;
        int seg_idx = r->payload[0];
        unsigned offset = r->payload[1] | (r->payload[2] << 8);
        long data_len = (long)r->len - 3;
        long end = (long)offset + data_len;
        if (seg_sizes[seg_idx] < 0 || end > seg_sizes[seg_idx])
            seg_sizes[seg_idx] = end;
    }
}

/* Patch SEGDEF records with computed segment sizes (in place). */
static int patch_segdefs(RecordList *records, const long seg_sizes[256]) {
    int patched = 0;
    int segdef_idx = 0;
    for (size_t i = 0; i < records->count; i++) {
        Record *r = &records->items[i];
        if (r->type != REC_SEGDEF)
            continue;
        segdef_idx++;
        unsigned old_len = r->payload[1] | (r->payload[2] << 8);
        long new_len = (segdef_idx < 256 && seg_sizes[segdef_idx] >= 0)
                           ? seg_sizes[segdef_idx]
                           : (long)old_len;
        if ((unsigned)new_len != old_len) {
            r->payload[1] = (uint8_t)(new_len & 0xFF);
            r->payload[2] = (uint8_t)((new_len >> 8) & 0xFF);
            printf("  Patched SEGDEF #%d: len %u -> %ld\n", segdef_idx, old_len, new_len);
            patched++;
        }
    }
    return patched;
}

/* Thread state: method/datum pair, or "undefined". */
typedef struct {
    int defined;
    int method;
    int datum; /* -1 means "no datum" */
} ThreadEntry;

/* Expand thread-based fixup references to explicit form.
 *
 * Thread subrecords define reusable frame/target references.
 * LINK86 v1.4 doesn't support them. This function:
 * - Tracks thread state across FIXUPP records
 * - Replaces F=1 (frame via thread) with F=0 (explicit frame)
 * - Replaces T=1 (target via thread) with T=0 (explicit target)
 * - Removes pure thread-definition FIXUPP records
 */
static void expand_fixupp_threads(const RecordList *records, RecordList *out,
                                   int *expanded_out, int *threads_removed_out) {
    /* Thread state persists across FIXUPP records within a module */
    ThreadEntry frame_threads[4] = {0};
    ThreadEntry target_threads[4] = {0};

    int expanded = 0;
    int threads_removed = 0;

    for (size_t ri = 0; ri < records->count; ri++) {
        const Record *rec = &records->items[ri];
        if (rec->type != REC_FIXUPP) {
            rl_push(out, rec->type, rec->payload, rec->len);
            continue;
        }

        const uint8_t *payload = rec->payload;
        size_t plen = rec->len;
        size_t idx = 0;
        ByteBuf fixups; /* concatenated expanded fixup subrecords */
        buf_init(&fixups);
        int has_fixups = 0;

        while (idx < plen) {
            if (payload[idx] & 0x80) {
                /* --- Fixup subrecord --- */
                has_fixups = 1;
                uint8_t locat_hi = payload[idx];
                uint8_t locat_lo = payload[idx + 1];
                uint8_t fix_data = payload[idx + 2];
                int f_bit = (fix_data >> 7) & 1;
                int t_bit = (fix_data >> 3) & 1;

                size_t idx2 = idx + 3;

                int frame_method, frame_datum = -1;
                int target_method, target_datum = -1;
                int disp_present = 0;
                uint8_t disp[2] = {0, 0};

                /* Parse/resolve frame */
                if (!f_bit) {
                    frame_method = (fix_data >> 4) & 7;
                    if (frame_method <= 2) {
                        frame_datum = payload[idx2];
                        idx2 += 1;
                    }
                } else {
                    int frame_thread_num = (fix_data >> 4) & 3;
                    if (!frame_threads[frame_thread_num].defined) {
                        printf("  WARNING: frame thread #%d not defined\n", frame_thread_num);
                        frame_method = 5; /* fallback: target frame */
                    } else {
                        frame_method = frame_threads[frame_thread_num].method;
                        frame_datum = frame_threads[frame_thread_num].datum;
                    }
                }

                /* Parse/resolve target */
                if (!t_bit) {
                    target_method = fix_data & 7;
                    if (target_method <= 6) {
                        target_datum = payload[idx2];
                        idx2 += 1;
                    }
                    /* Displacement present for methods 0-3 */
                    if (target_method <= 3) {
                        disp[0] = payload[idx2];
                        disp[1] = payload[idx2 + 1];
                        idx2 += 2;
                        disp_present = 1;
                    }
                } else {
                    /* T=1: target from thread.
                     * fix_data bits 2-0: bit 2 = P (no displacement if 1),
                     *                    bits 1-0 = thread number */
                    int p_bit = (fix_data >> 2) & 1;
                    int target_thread_num = fix_data & 3;
                    if (!target_threads[target_thread_num].defined) {
                        printf("  WARNING: target thread #%d not defined\n", target_thread_num);
                        target_method = 4; /* fallback */
                    } else {
                        int thread_method = target_threads[target_thread_num].method;
                        target_datum = target_threads[target_thread_num].datum;
                        if (p_bit) {
                            target_method = thread_method + 4;
                        } else {
                            target_method = thread_method;
                            disp[0] = payload[idx2];
                            disp[1] = payload[idx2 + 1];
                            idx2 += 2;
                            disp_present = 1;
                        }
                    }
                }

                /* Build expanded fixup subrecord (F=0, T=0) */
                uint8_t new_fix_data = (uint8_t)(((frame_method & 7) << 4) | (target_method & 7));

                buf_push(&fixups, locat_hi);
                buf_push(&fixups, locat_lo);
                buf_push(&fixups, new_fix_data);

                /* Frame datum (methods 0-2 have datum, 3+ don't) */
                if (frame_method <= 2 && frame_datum != -1)
                    buf_push(&fixups, (uint8_t)frame_datum);

                /* Target datum (methods 0-6 have datum, 7 doesn't) */
                if (target_method <= 6 && target_datum != -1)
                    buf_push(&fixups, (uint8_t)target_datum);

                /* Displacement (methods 0-3 only) */
                if (disp_present)
                    buf_append(&fixups, disp, 2);

                if (f_bit || t_bit)
                    expanded++;

                idx = idx2;
            } else {
                /* --- Thread subrecord --- */
                int d = (payload[idx] >> 6) & 1;
                int method = (payload[idx] >> 2) & 7;
                int thread_num = payload[idx] & 3;
                size_t idx2 = idx + 1;

                int datum = -1;
                if (method <= 2) {
                    datum = payload[idx2];
                    idx2 += 1;
                }

                ThreadEntry *table = d == 0 ? frame_threads : target_threads;
                table[thread_num].defined = 1;
                table[thread_num].method = method;
                table[thread_num].datum = datum;

                idx = idx2;
            }
        }

        if (has_fixups) {
            /* Rebuild the FIXUPP record with only expanded fixup subrecords */
            rl_push(out, REC_FIXUPP, fixups.data, fixups.len);
        } else {
            /* Pure thread-definition record, drop it */
            threads_removed++;
        }
        buf_free(&fixups);
    }

    *expanded_out = expanded;
    *threads_removed_out = threads_removed;
}

static const char *basename_of(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static int fix_obj_file(const char *input_path, const char *output_path) {
    FILE *f = fopen(input_path, "rb");
    if (!f) {
        fprintf(stderr, "  ERROR: cannot open %s\n", input_path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(fsize > 0 ? (size_t)fsize : 1);
    size_t nread = fread(data, 1, (size_t)fsize, f);
    fclose(f);

    RecordList records = parse_records(data, nread);
    free(data);

    if (records.count == 0) {
        printf("  ERROR: No records found in %s\n", input_path);
        rl_free(&records);
        return 0;
    }

    if (records.items[records.count - 1].type != REC_MODEND) {
        printf("  ERROR: No MODEND record found in %s\n", input_path);
        rl_free(&records);
        return 0;
    }

    /* Step 1: Compute segment sizes from LEDATA records */
    long seg_sizes[256];
    compute_seg_sizes(&records, seg_sizes);

    /* Step 2: Patch SEGDEF records */
    int seg_patches = patch_segdefs(&records, seg_sizes);

    /* Step 3: Expand thread-based fixups */
    RecordList expanded_records;
    rl_init(&expanded_records);
    int fixups_expanded = 0, threads_removed = 0;
    expand_fixupp_threads(&records, &expanded_records, &fixups_expanded, &threads_removed);
    rl_free(&records);

    /* Step 4: Write output records */
    ByteBuf buf;
    buf_init(&buf);
    for (size_t i = 0; i < expanded_records.count; i++) {
        Record *r = &expanded_records.items[i];
        write_record(&buf, r->type, r->payload, r->len);
    }
    rl_free(&expanded_records);

    /* Step 5: Pad to 128-byte boundary (required by CP/M-86) */
    size_t pad = 128 - (buf.len % 128);
    if (pad < 128) {
        for (size_t i = 0; i < pad; i++)
            buf_push(&buf, 0x00);
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        fprintf(stderr, "  ERROR: cannot write %s\n", output_path);
        buf_free(&buf);
        return 0;
    }
    fwrite(buf.data, 1, buf.len, out);
    fclose(out);

    const char *basename = basename_of(input_path);
    if (seg_patches == 0 && fixups_expanded == 0) {
        printf("  %s: no patches needed\n", basename);
    } else {
        printf("  %s: patched", basename);
        int first = 1;
        if (seg_patches) {
            printf("%s%d SEGDEF(s)", first ? " " : ", ", seg_patches);
            first = 0;
        }
        if (fixups_expanded) {
            printf("%s%d fixup(s) expanded", first ? " " : ", ", fixups_expanded);
            first = 0;
        }
        if (threads_removed) {
            printf("%s%d thread record(s) removed", first ? " " : ", ", threads_removed);
            first = 0;
        }
        printf("\n");
    }
    printf("  %s: output %zu bytes (128-byte aligned)\n", basename, buf.len);

    buf_free(&buf);
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.obj> [output.obj]\n", argv[0]);
        return 1;
    }
    const char *input_path = argv[1];
    const char *output_path = argc > 2 ? argv[2] : argv[1];
    if (!fix_obj_file(input_path, output_path))
        return 1;
    return 0;
}
