/*
 * Rendering of a plan_t for -### and -v.  See plan.h.
 *
 * The format is a stable contract, checked byte for byte by the golden
 * tests in tests/plan:
 *
 *   acc: plan stage=S target=T ...         every resolved setting
 *                                         (target=none: OS-neutral compile)
 *   acc: E: ...                           meaning of the E: drive
 *   tool: "PROGRAM" "ARG" ...             one line per backend command
 *   response: "FILE" "LINE" ...           linker response file, if any
 *   publish: "PRODUCT" -> "OUTPUT"        one line per published output
 *
 * Every word is double-quoted, with '"' and '\' escaped by a backslash
 * and control characters written as \xNN, so the output is unambiguous.
 */
#include "acc.h"
#include "request.h"
#include "plan.h"
#include "profile.h"

/* Names of settings that have no option value list, indexed by enum. */
static const char *const stage_names[] = { "link", "object", "assembly" };
static const char *const optimize_names[] = { "default", "size", "speed" };

/* Write TEXT to stderr as a quoted, escaped word. */
static void quoted(const char *text)
{
    fputc('"', stderr);
    while (*text != '\0') {
        if (*text == '"' || *text == '\\')
            fputc('\\', stderr);
        if ((unsigned char)*text < 32)
            fprintf(stderr, "\\x%02x", (unsigned int)(unsigned char)*text);
        else
            fputc(*text, stderr);
        ++text;
    }
    fputc('"', stderr);
}

/* Write " WORD" for every entry of WORDS from index FIRST on. */
static void quoted_words(const word_list_t *words, int first)
{
    int i;

    for (i = first; i < words->count; ++i) {
        fputc(' ', stderr);
        quoted(words->items[i]);
    }
}

/* Write PLAN, built from R, to stderr in the format described above. */
void print_plan(const request_t *r, const plan_t *plan)
{
    const command_t *cmd;
    int i;

    fprintf(stderr, "acc: plan stage=%s target=%s model=%s cpu=%s\n",
            stage_names[r->stage],
            plan->target_known ? target_names[plan->target] : "none",
            model_names[r->model], cpu_names[r->cpu]);
    fprintf(stderr, "acc: plan float=%s optimize=%s debug=%s\n",
            float_names[r->floating], optimize_names[r->optimize],
            r->debug ? "on" : "off");
    fputs(r->dryrun ?
          "acc: E: is a symbolic private workspace; no files are created\n" :
          "acc: E: maps the invocation's private workspace\n", stderr);
    for (cmd = plan->first; cmd != 0; cmd = cmd->next) {
        fputs("tool:", stderr);
        quoted_words(&cmd->arguments, 0);
        fputc('\n', stderr);
        if (cmd->response.count != 0) {
            /* response.items[0] is the tool; the lines follow it. */
            fputs("response: ", stderr);
            quoted(cmd->response_file);
            quoted_words(&cmd->response, 1);
            fputc('\n', stderr);
        }
    }
    for (i = 0; i < plan->outputs.count; ++i) {
        fputs("publish: ", stderr);
        quoted(plan->products.items[i]);
        fputs(" -> ", stderr);
        quoted(plan->outputs.items[i]);
        fputc('\n', stderr);
    }
}
