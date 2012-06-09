
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hook.h"
#include "globals.h"
#include "threading.h"
#include "memory_.h"

// A table defining the hooks' state, which currently consists of the
// information which groups are enabled. The global hook state is defined in
// globals.h
struct __hlt_hook_state {
    int64_t size;   // Current size of integer array.
    int32_t* state; // Array of integers, defining a bitset indicating each group's state.
};

static void _fatal_error(const char* msg)
{
    fprintf(stderr, "libhilti hooks: %s\n", msg);
    exit(1);
}

void __hlt_hooks_init()
{
    if ( __hlt_global_hook_state )
        return;

    __hlt_global_hook_state = hlt_malloc(sizeof(hlt_hook_state));
    __hlt_global_hook_state->state = 0;
    __hlt_global_hook_state->size = 0;

    if ( hlt_is_multi_threaded() && pthread_mutex_init(&__hlt_global_hook_state_lock, 0) != 0 )
        _fatal_error("cannot initialize worker's lock");
}

void __hlt_hooks_done()
{
    if ( hlt_is_multi_threaded() && pthread_mutex_destroy(&__hlt_global_hook_state_lock) != 0 )
        _fatal_error("could not destroy lock");

    hlt_free(__hlt_global_hook_state->state);
    hlt_free(__hlt_global_hook_state);
}

void hlt_hook_group_enable(int64_t group, int8_t enabled, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(group >= 0);
    assert(__hlt_global_hook_state);

    if ( hlt_is_multi_threaded() && pthread_mutex_lock(&__hlt_global_hook_state_lock) != 0 )
        _fatal_error("cannot acquire lock");

    int64_t n = group / 32;

    if ( n >= __hlt_global_hook_state->size ) {
        int64_t old_size = __hlt_global_hook_state->size;
        int64_t new_size = n + 1;

        __hlt_global_hook_state->state = hlt_realloc(__hlt_global_hook_state->state, new_size * sizeof(int32_t));

        // Default is enabled.
        memset(__hlt_global_hook_state->state + old_size, 255, (new_size - old_size)  * sizeof(int32_t));
        __hlt_global_hook_state->size = new_size;
    }

    int64_t bit = 1L << (group - (n * 32));

    if ( enabled )
        __hlt_global_hook_state->state[n] |= bit;
    else
        __hlt_global_hook_state->state[n] &= ~bit;

exit:
    if ( hlt_is_multi_threaded() && pthread_mutex_unlock(&__hlt_global_hook_state_lock) != 0 )
        _fatal_error("cannot release lock");
}

int8_t hlt_hook_group_is_enabled(int64_t group, hlt_exception** excpt, hlt_execution_context* ctx)
{
    // TODO: We don't lock here, which means we have a slight race condition
    // around the time somebody enables/disabled a hook. Does it matter?

    assert(group >= 0);

    int64_t n = group / 32;

    if ( (! __hlt_global_hook_state) || n >= __hlt_global_hook_state->size )
        // We don't have disabled that group yet, so it's enabled.
        return 1;

    int64_t bit = 1L << (group - (n * 32));

    return (__hlt_global_hook_state->state[n] & bit) != 0;
}

