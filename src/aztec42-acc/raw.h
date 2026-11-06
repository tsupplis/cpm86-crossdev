/*
 * Raw backend options: -Wc,ARG (cc), -Wa,ARG (as) and -Wl,ARG (ln).
 *
 * Each ARG is one backend option, checked against an allow-list (plan.md
 * section 5): options the driver manages itself, foreign ABI modes and
 * options with unowned side artifacts are refused, so a raw option can
 * never contradict the plan.
 */
#ifndef RAW_H
#define RAW_H

#include "acc.h"

typedef enum { RAW_COMPILER, RAW_ASSEMBLER, RAW_LINKER } raw_tool_t;

/* Facts about the invocation that some options depend on. */
typedef struct {
    int assembly_stage;         /* -S */
    int large_data;             /* compact or large model */
    int dos_link;               /* link target is DOS 1 or DOS 2 */
    int dos2_exe;               /* link output is a DOS 2 .exe */
} raw_context_t;

/*
 * Check ARG for TOOL in CONTEXT and append it to ARGS, as one argument or,
 * for linker options with an operand, as option and operand.
 * Returns 0 after a diagnostic naming ARG.
 */
int raw_append(word_list_t *args, raw_tool_t tool, const char *arg,
               const raw_context_t *context);

#endif
