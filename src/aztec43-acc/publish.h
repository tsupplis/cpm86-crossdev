#ifndef PUBLISH_H
#define PUBLISH_H

#include "plan.h"

/* Stage and backup return an owned path, or an empty string if none.
 * The buffers have room for the destination path plus 16 bytes.
 * Restore replaces an installed output if necessary. */
typedef struct {
    int (*stage)(char *, const char *, const char *, const char *);
    int (*backup)(char *, const char *);
    int (*move)(const char *, const char *);
    int (*restore)(const char *, const char *, int);
    int (*remove)(const char *);
    int (*cancelled)(void);
} publish_ops_t;

int publish_outputs(const plan_t *plan, const char *work,
                    const publish_ops_t *ops);

#endif
