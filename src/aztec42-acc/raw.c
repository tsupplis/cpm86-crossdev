/*
 * Raw backend option allow-list.  See raw.h.
 *
 * Spellings follow the Aztec C86 4.10 manual and were checked against the
 * shipped cc, as and ln: cc and as take joined operands ("-L300", "-S3"),
 * ln takes separate ones ("-S 400").  Operand values are positive and fit
 * a 16-bit int (decimal) or one segment (hexadecimal, up to 4 digits).
 */
#include "acc.h"
#include "raw.h"
#include <string.h>
#include <ctype.h>

typedef enum { OPERAND_NONE, OPERAND_DECIMAL, OPERAND_HEX } operand_t;

/* Context requirements, combined as a bit mask. */
#define NEEDS_ASSEMBLY 1        /* only with -S */
#define NEEDS_LARGE_DATA 2      /* only with compact or large model */
#define NEEDS_DOS 4             /* only when linking for DOS */
#define NEEDS_DOS2_EXE 8        /* only when linking a DOS 2 .exe */

typedef struct {
    raw_tool_t tool;
    const char *spelling;       /* option, without its operand */
    operand_t operand;
    int separate;               /* operand is a separate argument */
    int needs;                  /* NEEDS_* mask */
} raw_option_t;

static const raw_option_t options[] = {
    /* cc table sizes: expressions, locals, switch cases, strings. */
    { RAW_COMPILER, "-E", OPERAND_DECIMAL, 0, 0 },
    { RAW_COMPILER, "-L", OPERAND_DECIMAL, 0, 0 },
    { RAW_COMPILER, "-y", OPERAND_DECIMAL, 0, 0 },
    { RAW_COMPILER, "-z", OPERAND_DECIMAL, 0, 0 },
    /* cc: C source as comments in the assembly output. */
    { RAW_COMPILER, "-T", OPERAND_NONE, 0, NEEDS_ASSEMBLY },
    /* cc: no near/far/huge/pascal/fortran/cdecl keywords. */
    { RAW_COMPILER, "-k", OPERAND_NONE, 0, 0 },
    /* cc: uninitialised globals as externs, no register tracking,
     * no-alias assertion, MUL for constant multiplication. */
    { RAW_COMPILER, "+U", OPERAND_NONE, 0, 0 },
    { RAW_COMPILER, "+R", OPERAND_NONE, 0, 0 },
    { RAW_COMPILER, "+A", OPERAND_NONE, 0, 0 },
    { RAW_COMPILER, "+M", OPERAND_NONE, 0, 0 },
    /* cc: strings in the code segment (large data models only). */
    { RAW_COMPILER, "+s", OPERAND_NONE, 0, NEEDS_LARGE_DATA },
    /* as: squeeze passes; listing to the console. */
    { RAW_ASSEMBLER, "-S", OPERAND_DECIMAL, 0, 0 },
    { RAW_ASSEMBLER, "-C", OPERAND_NONE, 0, 0 },
    { RAW_ASSEMBLER, "-CA", OPERAND_NONE, 0, 0 },
    { RAW_ASSEMBLER, "-CS", OPERAND_NONE, 0, 0 },
    /* ln: no multiple-definition warnings; verbose. */
    { RAW_LINKER, "-M", OPERAND_NONE, 0, 0 },
    { RAW_LINKER, "-V", OPERAND_NONE, 0, 0 },
    /* ln: DOS minimum stack+heap bytes; DOS 2 maximum paragraphs (hex). */
    { RAW_LINKER, "-S", OPERAND_HEX, 1, NEEDS_DOS },
    { RAW_LINKER, "-X", OPERAND_HEX, 1, NEEDS_DOS2_EXE }
};

#define OPTION_COUNT (sizeof(options) / sizeof(options[0]))

/* Non-zero if TEXT is a decimal number from 1 to 32767. */
static int valid_decimal(const char *text)
{
    long value;

    if (*text == '\0' || strlen(text) > 5)
        return 0;
    value = 0;
    for (; *text != '\0'; ++text) {
        if (!isdigit((unsigned char)*text))
            return 0;
        value = value * 10 + (*text - '0');
    }
    return value >= 1 && value <= 32767;
}

/* Non-zero if TEXT is 1 to 4 hexadecimal digits, not all zero. */
static int valid_hex(const char *text)
{
    int nonzero;
    size_t length;

    length = strlen(text);
    if (length == 0 || length > 4)
        return 0;
    nonzero = 0;
    for (; *text != '\0'; ++text) {
        if (!isxdigit((unsigned char)*text))
            return 0;
        if (*text != '0')
            nonzero = 1;
    }
    return nonzero;
}

/* Diagnostic for an unmet NEEDS_* requirement of option ARG. */
static int unmet(int needs, const char *arg)
{
    if (needs & NEEDS_ASSEMBLY)
        return fail("backend option needs -S", arg);
    if (needs & NEEDS_LARGE_DATA)
        return fail("backend option needs -mcmodel=compact or large", arg);
    if (needs & NEEDS_DOS)
        return fail("backend option needs a DOS link", arg);
    return fail("backend option needs a dos2 .exe link", arg);
}

/* Find ARG's entry for TOOL, check operand and context, then append. */
int raw_append(word_list_t *args, raw_tool_t tool, const char *arg,
               const raw_context_t *context)
{
    const raw_option_t *option;
    const char *operand;
    size_t length;
    size_t i;
    int unmet_needs;

    for (i = 0; i < OPTION_COUNT; ++i) {
        option = &options[i];
        length = strlen(option->spelling);
        if (option->tool != tool || strncmp(arg, option->spelling, length) != 0)
            continue;
        operand = arg + length;
        if (option->operand == OPERAND_NONE ? *operand != '\0' :
            option->operand == OPERAND_DECIMAL ? !valid_decimal(operand) :
            !valid_hex(operand))
            continue;
        unmet_needs = option->needs;
        if (context->assembly_stage)
            unmet_needs &= ~NEEDS_ASSEMBLY;
        if (context->large_data)
            unmet_needs &= ~NEEDS_LARGE_DATA;
        if (context->dos_link)
            unmet_needs &= ~NEEDS_DOS;
        if (context->dos2_exe)
            unmet_needs &= ~NEEDS_DOS2_EXE;
        if (unmet_needs != 0)
            return unmet(unmet_needs, arg);
        if (!option->separate)
            return add_word(args, arg);
        return add_word(args, option->spelling) && add_word(args, operand);
    }
    return fail("backend option is not allowed or its operand is invalid", arg);
}
