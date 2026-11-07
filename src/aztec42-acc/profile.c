/*
 * Target profile tables.  See profile.h.
 *
 * Library and startup names are those of the shipped Aztec C86 4.10
 * runtime in the tool root's lib/; flag spellings were checked against the
 * cc and as binaries.  CP/M-86 and DOS 1 support only the small model
 * (manual, Tech Info 1.1.1), so their entries are single names; DOS 2
 * tables are indexed by model.  A DOS .com program is a small-model program
 * and uses the same startup and runtime as a small .exe.
 */
#include "acc.h"
#include "profile.h"
#include "path.h"
#include <string.h>

const char *const target_names[TARGET_COUNT] = { "cpm86", "dos1", "dos2" };
const char *const model_names[MODEL_COUNT] = {
    "small", "medium", "compact", "large"
};
const char *const cpu_names[CPU_COUNT] = { "i8086", "i80186", "i80286" };
const char *const float_names[FLOAT_COUNT] = {
    "soft", "auto", "8087"
};

/* cc flags, indexed by enum; 0 means "no flag". */
static const char *const cpu_flags[] = { "+0", "+1", "+2" };
static const char *const model_flags[] = { 0, "+lc", "+ld", "+l" };
/* No unsafe +ej mode: cc v4.10d rejects it ("unknown option: j"). */
static const char *const float_flags[] = { 0, 0, "+e" };
static const char *const optimize_flags[] = { 0, "+c", "+f" };

/* Indexed by Target; the value of __MSDOS__ is the DOS major version. */
static const char *const target_macros[] = {
    "-D__CPM86__=1", "-D__MSDOS__=1", "-D__MSDOS__=2"
};
static const char *const executables[] = { "a.cmd", "a.exe", "a.exe" };

/* DOS 2 startup objects and C runtimes, indexed by Model. */
static const char *const dos2_startups[] = {
    "sbegin.o", "beginlc.o", "beginld.o", "lbegin.o"
};
static const char *const dos2_runtimes[] = {
    "c.lib", "clc.lib", "cld.lib", "cl.lib"
};

/*
 * Math libraries, indexed [family][Model]: software, sensing (uses an 8087
 * when one is present) and hardware.
 */
static const char *const maths[3][MODEL_COUNT] = {
    { "m.lib", "mlc.lib", "mld.lib", "ml.lib" },
    { "m87s.lib", "m87slc.lib", "m87sld.lib", "m87sl.lib" },
    { "m87.lib", "m87lc.lib", "m87ld.lib", "m87l.lib" }
};

/* -lNAME values naming a stock C runtime of some profile. */
static const char *const runtimes[] = { "c", "c86", "d11", "cl", "clc", "cld" };

/* Look VALUE up in NAMES (exact, case-sensitive match, like every option). */
int profile_choice(const char *value, const char *const *names, int count)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (strcmp(value, names[i]) == 0)
            return i;
    }
    fail("invalid option value", value);
    return -1;
}

/* cc code generation flag for CPU: +0, +1 or +2. */
const char *profile_cpu_flag(cpu_t cpu)
{
    return cpu_flags[cpu];
}

/* cc memory model flag; the small model needs none. */
const char *profile_model_flag(model_t model)
{
    return model_flags[model];
}

/* cc inline coprocessor flag; software and sensing math need none. */
const char *profile_float_flag(float_backend_t floating)
{
    return float_flags[floating];
}

/* cc code preference flag: +c (compact), +f (fast), or none. */
const char *profile_optimize_flag(optimize_t optimize)
{
    return optimize_flags[optimize];
}

/*
 * as flag for CPU: -186 for the 80186 and the 80286, whose real-mode
 * instruction set is the 80186's; as has no separate 80286 option.
 */
const char *profile_assembler_cpu_flag(cpu_t cpu)
{
    return cpu == CPU_I8086 ? 0 : "-186";
}

/*
 * as coprocessor flag: 8087 instructions always assemble, with a WAIT
 * inserted before each.  With an 80286 and inline floating point, -2
 * selects 80287 code without those WAITs, as cc +2 +e does itself
 * (release notes: compiler support for inline 80287 instructions).
 */
const char *profile_assembler_float_flag(cpu_t cpu, float_backend_t floating)
{
    return cpu == CPU_I80286 &&
        floating == FLOAT_8087 ? "-2" : 0;
}

/* -D argument identifying the target operating system. */
/* Compiler identity macro, defined for every compilation (C and assembler). */
const char *profile_identity_macro(void)
{
    return "-D__AZTEC__=1";
}

const char *profile_target_macro(target_t target)
{
    return target_macros[target];
}

/* CPU tier macros, indexed by cpu_t; each list ends with 0. */
static const char *const cpu_macros_8086[] = { 0 };
static const char *const cpu_macros_80186[] = { "-D__i186__=1", 0 };
static const char *const cpu_macros_80286[] = {
    "-D__i186__=1", "-D__i286__=1", 0
};
static const char *const *const cpu_macros[] = {
    cpu_macros_8086, cpu_macros_80186, cpu_macros_80286
};

/* -D arguments for the -march tier (none for the 8086). */
const char *const *profile_cpu_macros(cpu_t cpu)
{
    return cpu_macros[cpu];
}

/* a.cmd for CP/M-86, a.exe for DOS. */
const char *profile_default_executable(target_t target)
{
    return executables[target];
}

/* Startup object: sets up segments and the stack, then calls main. */
const char *profile_startup(target_t target, model_t model)
{
    if (target == TARGET_CPM86)
        return "begin86.o";
    if (target == TARGET_DOS1)
        return "begin11.o";
    return dos2_startups[model];
}

/* C runtime library, linked after every user input. */
const char *profile_runtime(target_t target, model_t model)
{
    if (target == TARGET_CPM86)
        return "c86.lib";
    if (target == TARGET_DOS1)
        return "d11.lib";
    return dos2_runtimes[model];
}

/* Math library selected by -lm for the floating backend and model. */
const char *profile_math(float_backend_t floating, model_t model)
{
    return maths[floating][model];
}

/* Compared as DOS file names: -lC86 and -lc86 name the same library. */
int profile_is_runtime(const char *name)
{
    size_t i;

    for (i = 0; i < sizeof(runtimes) / sizeof(runtimes[0]); ++i) {
        if (path_equal(name, runtimes[i]))
            return 1;
    }
    return 0;
}
