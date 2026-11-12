/* Stage the complete output set before changing any destination. */
#include "publish.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *staged;
    char *backup;
    int installed;
} publication_t;

int publish_outputs(const plan_t *plan, const char *work,
                    const publish_ops_t *ops)
{
    publication_t *entries;
    publication_t *entry;
    const char *dest;
    size_t capacity;
    int count;
    int i;
    int ok;

    count = plan->outputs.count;
    if (count == 0)
        return 1;
    entries = calloc(count, sizeof(publication_t));
    if (entries == 0)
        return fail("out of memory", 0);
    ok = 1;
    for (i = 0; ok && !ops->cancelled() && i < count; ++i) {
        entry = &entries[i];
        dest = plan->host_outputs.items[i];
        capacity = strlen(dest) + 16;
        entry->staged = calloc(capacity, 1);
        entry->backup = calloc(capacity, 1);
        if (entry->staged == 0 || entry->backup == 0)
            ok = fail("out of memory", 0);
        else
            ok = ops->stage(entry->staged, work, plan->products.items[i], dest);
    }
    for (i = 0; ok && !ops->cancelled() && i < count; ++i) {
        entry = &entries[i];
        dest = plan->host_outputs.items[i];
        ok = ops->backup(entry->backup, dest);
        if (ok && !ops->cancelled()) {
            ok = ops->move(entry->staged, dest);
            if (ok) {
                entry->staged[0] = '\0';
                entry->installed = 1;
            }
        }
    }
    if (ops->cancelled())
        ok = 0;
    if (!ok) {
        for (i = count - 1; i >= 0; --i) {
            entry = &entries[i];
            dest = plan->host_outputs.items[i];
            if (entry->backup != 0 && *entry->backup != '\0') {
                if (!ops->restore(entry->backup, dest, entry->installed))
                    fprintf(stderr, "acc: previous output retained at %s\n",
                            entry->backup);
                /* A failed restoration leaves the backup for recovery. */
                entry->backup[0] = '\0';
            } else if (entry->installed) {
                ops->remove(dest);
            }
        }
    }
    for (i = 0; i < count; ++i) {
        entry = &entries[i];
        if (entry->staged != 0 && *entry->staged != '\0' &&
            !ops->remove(entry->staged))
            ok = 0;
        if (entry->backup != 0 && *entry->backup != '\0' &&
            !ops->remove(entry->backup))
            ok = 0;
        free(entry->staged);
        free(entry->backup);
    }
    free(entries);
    return ok;
}
