/*
 * The parsed invocation.
 *
 * parse_request checks syntax only (spelling, operands, exclusive
 * switches, settings that contradict each other).  It never touches the
 * filesystem: files and the installation are validated by build_plan.
 */
#ifndef REQUEST_H
#define REQUEST_H

#include "acc.h"
#include "profile.h"

/*
 * Zero-initialised means every default: link stage, no explicit target,
 * small model, 8086, software floating point, native optimisation.
 */
typedef struct {
    stage_t stage;
    target_t target;            /* meaningful only when target_given */
    int target_given;           /* -target was given */
    model_t model;
    cpu_t cpu;
    float_backend_t floating;
    optimize_t optimize;
    int strict_ansi;            /* -ansi: strict ANSI, no near/far/huge */
    int debug;                  /* -g: sdb information and .dbg file */
    int quiet;                  /* -w: suppress compiler warnings */
    int nostart;                /* -nostartfiles */
    int nostdlib;               /* -nostdlib */
    int dryrun;                 /* -###: print the plan, run nothing */
    int verbose;                /* -v: print the plan, then run it */
    int save_temps;             /* -save-temps: keep the workspace */
    char *output;               /* -o, or 0 */
    char *program;              /* argv[0], for tool root discovery */
    char *root;                 /* -toolroot, or 0 (then discovery, ACCROOT) */
    char *emulator;             /* -emu (Unix only), or 0 (then emu2 in PATH) */
    char *overlay_root;         /* -overlay-root CODE,DATA value, or 0 */
    char *overlay_table;        /* -overlay ROOT.rsm path, or 0 */
    word_list_t includes;       /* -I areas, in order */
    word_list_t definitions;    /* -D macros, normalised to NAME=VALUE */
    word_list_t libraries;      /* -L areas, in order */
    word_list_t inputs;         /* files and "-lNAME" entries, in order */
    word_list_t cc_options;     /* -Wc, raw compiler options, in order */
    word_list_t as_options;     /* -Wa, raw assembler options, in order */
    word_list_t ln_options;     /* -Wl, raw linker options, in order */
} request_t;

/*
 * Parse ARGV into R, which must be zero-initialised.
 * Returns 0 after a diagnostic.
 */
int parse_request(request_t *r, int argc, char **argv);

/* Release everything owned by R. */
void free_request(request_t *r);

#endif
