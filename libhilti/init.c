
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "config.h"
#include "debug.h"
#include "globals.h"
#include "hook.h"
#include "threading.h"
#include "profiler.h"
#include "init.h"

// Debugging support to flag wrong usage of hlt_init/hlt_exit.
static int _init_called = 0;
static int _done_called = 0;

static void __hlt_init_common()
{
    // Initialize locale.
    if ( ! setlocale(LC_CTYPE, "") ) {
        fprintf(stderr, "libhilti: cannot set locale");
        exit(1);
    }
}

void hlt_init()
{
    if ( _init_called )
        return;

    _init_called = 1;
    __hlt_init_common();

    __hlt_global_state_init(1);

    hlt_execution_context* ctx = hlt_global_execution_context();
    __hlt_modules_init(ctx);

    atexit(hlt_done);
}

void __hlt_init_from_state(__hlt_global_state* state)
{
#if 0
   if ( __hlt_globals() ) {
        fprintf(stderr, "internal error: __hlt_init_from_state() called when globals are already initialized\n");
        abort();
    }
#endif

    __hlt_init_common();
    __hlt_globals_set(state);

    // hlt_execution_context* ctx = hlt_global_execution_context();
    // __hlt_modules_init(ctx);

    _init_called = 1;
    _done_called = 1; // Don't clean up.
}

void hlt_done()
{
    if ( ! _init_called )
        return;

    if ( _done_called )
        return;

    _done_called = 1;

    hlt_exception* excpt = 0;

    __hlt_global_state_done();
}
