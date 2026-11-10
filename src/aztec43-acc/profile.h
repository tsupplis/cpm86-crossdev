/*
 * Target profiles: every fact about the Aztec C86 runtime and backend
 * flags that depends on the selected target, memory model, CPU, floating
 * point backend or optimisation preference.  acc compiles ANSI C only:
 * cc's native mode (ANSI plus the near/far/huge extensions used by the
 * Aztec headers); never -ansi (disables near/far) or -3 (v3.40b K&R rules).
 *
 * The planner asks these functions instead of spelling backend flags or
 * library names itself, so each mapping lives in exactly one place.
 * Functions returning a flag return 0 when the setting needs no flag.
 */
#ifndef PROFILE_H
#define PROFILE_H

/* Final product of an invocation. */
typedef enum {
    STAGE_LINK,         /* default: build an executable */
    STAGE_OBJECT,       /* -c: compile or assemble to objects */
    STAGE_ASSEMBLY      /* -S: compile to assembly */
} stage_t;

/* Values of -target, -mcmodel, -march and -mfloat, in option order. */
typedef enum { TARGET_CPM86, TARGET_DOS1, TARGET_DOS2 } target_t;
typedef enum { MODEL_SMALL, MODEL_MEDIUM, MODEL_COMPACT, MODEL_LARGE } model_t;
typedef enum { CPU_I8086, CPU_I80186, CPU_I80286 } cpu_t;
typedef enum { FLOAT_SOFT, FLOAT_AUTO, FLOAT_8087 } float_backend_t;

/* -Osize / -Ospeed; DEFAULT keeps the compiler's native preference. */
typedef enum { OPTIMIZE_DEFAULT, OPTIMIZE_SIZE, OPTIMIZE_SPEED } optimize_t;

/* Option spellings, indexed by the enums above. */
#define TARGET_COUNT 3
#define MODEL_COUNT 4
#define CPU_COUNT 3
#define FLOAT_COUNT 3
extern const char *const target_names[TARGET_COUNT];
extern const char *const model_names[MODEL_COUNT];
extern const char *const cpu_names[CPU_COUNT];
extern const char *const float_names[FLOAT_COUNT];

/*
 * Return the index of VALUE in NAMES[0..COUNT-1], or -1 after an
 * "invalid option value" diagnostic.
 */
int profile_choice(const char *value, const char *const *names, int count);

/* Compiler (cc) flags. */
const char *profile_cpu_flag(cpu_t cpu);
const char *profile_model_flag(model_t model);
const char *profile_float_flag(float_backend_t floating);
const char *profile_optimize_flag(optimize_t optimize);

/* Assembler (as) flags for explicit .asm sources: CPU, then coprocessor. */
const char *profile_assembler_cpu_flag(cpu_t cpu);
const char *profile_assembler_float_flag(cpu_t cpu, float_backend_t floating);

/*
 * Preprocessor definition identifying the compiler family, as one argument:
 * -D__AZTEC__=1.  Defined for every compilation, with or without -target.
 */
const char *profile_identity_macro(void);

/*
 * Preprocessor definition identifying TARGET, as one cc argument:
 * -D__CPM86__=1, -D__MSDOS__=1 (dos1) or -D__MSDOS__=2 (dos2).  Defined
 * only for an explicit -target.
 */
const char *profile_target_macro(target_t target);

/*
 * Preprocessor definitions for CPU's tier, as cc/as arguments, ending with
 * 0: -D__i186__=1 for the 80186 and up, plus -D__i286__=1 for the 80286.
 * Defined by acc because cc has no signal for +1/+2 (verified).
 */
const char *const *profile_cpu_macros(cpu_t cpu);

/* Executable name used when -o is absent. */
const char *profile_default_executable(target_t target);

/*
 * File names, in the tool root's lib/, of the startup object, C runtime
 * and math library of a profile.
 */
const char *profile_startup(target_t target, model_t model);
const char *profile_runtime(target_t target, model_t model);
const char *profile_math(float_backend_t floating, model_t model);

/*
 * CP/M-86 command header flag bits (byte 7Fh) that FLOATING needs, or 0.
 * Bit 5 (0x20) allocates the 8087 to the program if one is present, which
 * the sensing math library (auto) uses.  Bit 6 (0x40) allocates it even if
 * absent, as the inline 8087 code (8087) requires.  Only a cpm86 program
 * has such a header.
 */
int profile_cmd_flags(target_t target, float_backend_t floating);

/*
 * The same bits for an explicit -lNAME: 0x60 for m87 (inline 8087 math),
 * 0x20 for m87s (sensing math), else 0.  The -lm shorthand is covered by
 * profile_cmd_flags through -mfloat.
 */
int profile_library_cmd_flags(target_t target, const char *name);

/* Non-zero if -lNAME names a stock C runtime (selected by the profile). */
int profile_is_runtime(const char *name);

#endif
