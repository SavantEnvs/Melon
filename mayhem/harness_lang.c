/* Fuzz the Melang interpreter: lexer, parser (AST), and VM execution of one script per input.
 * Safety filter, not a library precondition: scripts mentioning Import (dlopen of a host .so) or
 * Eval (reads a host file or spawns a job that can outlive the input) are rejected before the
 * library runs, because the lang is reused across inputs. Eval is the only way to call Import
 * without writing its name, so rejecting both closes the substring bypass. */
#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "mln_event.h"
#include "mln_lang.h"

static int fds[2];

static int lang_signal(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), fds[0], M_EV_SEND | M_EV_ONESHOT,
                            M_EV_UNLIMITED, lang, mln_lang_launcher_get(lang));
}

static int lang_clear(mln_lang_t *lang)
{
    return mln_event_fd_set(mln_lang_event_get(lang), fds[0], M_EV_CLR, M_EV_UNLIMITED, NULL, NULL);
}

static void on_return(mln_lang_ctx_t *ctx)
{
    mln_event_break_set((mln_event_t *)mln_lang_ctx_data_get(ctx));
}

/* Building the interpreter's parser tables costs about a second, so the lang is created once,
 * the way Melon's own t/lang.c runs every script on one lang; each input is a fresh job. */
static mln_event_t *ev;
static mln_lang_t *lang;

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) return -1;
    if ((ev = mln_event_new()) == NULL) return -1;
    if ((lang = mln_lang_new(ev, lang_signal, lang_clear)) == NULL) return -1;
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    mln_string_t src;

    if (size == 0 || memmem(data, size, "Import", 6) != NULL ||
        memmem(data, size, "Eval", 4) != NULL) return -1;
    mln_string_nset(&src, (mln_u8ptr_t)data, size);
    mln_event_break_reset(ev);
    if (mln_lang_job_new(lang, NULL, M_INPUT_T_BUF, &src, ev, on_return) == NULL) return -1;
    mln_event_dispatch(ev);
    return 0;
}
