/*
 * The execution plan: the exact backend commands of one invocation.
 *
 * A plan_t is pure data.  Every argument is in guest (DOS) form and refers to
 * the symbolic drives C: (invocation directory), D: (tool root) and E:
 * (private workspace).  Building a plan validates the whole request before
 * anything runs; the printed plan (-###) is the contract checked by the
 * golden tests in tests/plan.
 */
#ifndef PLAN_H
#define PLAN_H

#include "acc.h"
#include "request.h"

/*
 * Workspace temporaries are named E:\uNNNN.EXT from one counter per plan,
 * so every temporary of an invocation has its own name, and the private
 * workspace makes it unique across concurrent invocations.
 */
#define PLAN_TEMP_LIMIT 9999

/* One backend command. */
typedef struct command {
    word_list_t arguments;      /* tool path, then its arguments */
    word_list_t response;       /* if non-empty: tool path, then the lines
                                   of the response file */
    char *response_file;        /* guest path of that file, or 0 */
    char *product;              /* guest path the tool must create */
    char *source;               /* user-facing input, for diagnostics */
    struct command *next;
} command_t;

/* Everything needed to run and publish one invocation. */
typedef struct {
    command_t *first;           /* commands, in execution order */
    command_t *last;
    word_list_t outputs;        /* guest paths of published outputs */
    word_list_t products;       /* workspace products, parallel to outputs */
    word_list_t host_outputs;   /* host paths of published outputs */
    word_list_t library_paths;  /* guest library areas, for the child CLIB */
    int temps;                  /* workspace temporaries named so far */
    target_t target;            /* resolved target, if target_known */
    int target_known;           /* -target given, or link mode */
    char root[ACC_PATH];        /* host tool root (guest D:) */
    char cwd[ACC_PATH];         /* host invocation directory (guest C:) */
} plan_t;

/*
 * Validate R against the installation and the filesystem and fill PLAN,
 * which must be zero-initialised.  Creates nothing.  Returns 0 after a
 * diagnostic.
 */
int build_plan(const request_t *r, plan_t *plan);

/* Write PLAN to stderr in the documented -### format. */
void print_plan(const request_t *r, const plan_t *plan);

/* Release everything owned by PLAN. */
void free_plan(plan_t *plan);

/* Run PLAN and publish its outputs (execunix.c or execdos.c); returns the
 * exit status. */
int execute_plan(const request_t *r, const plan_t *plan);

#endif
