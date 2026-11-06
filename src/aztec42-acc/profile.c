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
    "soft", "auto", "8087", "8087-fast"
};

/* cc flags, indexed by enum; 0 means "no flag". */
static const char *const cpu_flags[] = { "+0", "+1", "+2" };
static const char *const model_flags[] = { 0, "+lc", "+ld", "+l" };
static const char *const float_flags[] = { 0, 0, "+e", "+ej" };
static const char *const optimize_flags[] = { 0, "+c", "+f" };

/* Indexed by Target; both DOS targets share one macro. */
static const char *const target_macros[] = {
    "-D__CPM86__=1", "-D__MSDOS__=1", "-D__MSDOS__=1"
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
 * when one is present) and hardware.  8087-fast uses the hardware family.
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
 * as flag for CPU.  Only -186 is known; explicit 80286 assembly is
 * rejected by the planner until its as behaviour is verified.
 */
const char *profile_assembler_cpu_flag(cpu_t cpu)
{
    return cpu == CPU_I80186 ? "-186" : 0;
}

/* -D argument identifying the target operating system. */
const char *profile_target_macro(target_t target)
{
    return target_macros[target];
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
    return maths[floating == FLOAT_8087_FAST ? FLOAT_8087 : floating][model];
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
