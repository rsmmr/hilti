//
// This follows roughly the idea from
// http://www.1024cores.net/home/lock-free-algorithms/tricks/fibers.
//

#include <setjmp.h>
#include <stdio.h>

#include "config.h"
#include "context.h"
#include "fiber.h"
#include "globals.h"
#include "hutil.h"
#include "memory_.h"
#include "threading.h"

#include "3rdparty/libtask/taskimpl.h"

enum __hlt_fiber_state { INIT, RUNNING, YIELDED, IDLE, FINISHED };

struct __hlt_fiber {
    enum __hlt_fiber_state state;
    ucontext_t uctx;
    jmp_buf fiber;
    jmp_buf trampoline;
    jmp_buf parent;
    void* cookie;
    void* result;
    hlt_execution_context* context;
    hlt_fiber_func run;
    struct __hlt_fiber* next; // If a member of fiber tool, subsequent fiber or null.
};

struct __hlt_fiber_pool {
    hlt_fiber* head;
    size_t size;
};

static void _fiber_trampoline(unsigned int y, unsigned int x)
{
    hlt_fiber* fiber;

    // Magic from from libtask/task.c to turn the two words back into a pointer.
    unsigned long z;
    z = (x << 16);
    z <<= 16;
    z |= y;
    fiber = (hlt_fiber*)z;

    // Via recycling a fiber can run an arbitrary number of user jobs. So
    // this trampoline is really a loop that yields after it has finished its
    // run() function, and expects a new run function once it's resumed.

    while ( 1 ) {
        assert(fiber->run);

        hlt_fiber_func run = fiber->run;
        void* cookie = fiber->cookie;

        assert(fiber->state == RUNNING);

        if ( ! _setjmp(fiber->trampoline) ) {
            (*run)(fiber, cookie);
        }

        if ( ! _setjmp(fiber->fiber) ) {
            fiber->run = 0;
            fiber->cookie = 0;
            fiber->state = IDLE;
            _longjmp(fiber->parent, 1);
        }
    }

    // Cannot be reached.
    abort();
}

static void fatal_error(const char* msg)
{
    fprintf(stderr, "fibers: %s\n", msg);
    exit(1);
}


static inline void acqire_lock(int* i)
{
    hlt_pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, i);

    if ( pthread_mutex_lock(&__hlt_globals()->synced_fiber_pool_lock) != 0 )
        fatal_error("cannot lock mutex");
}

static inline void release_lock(int i)
{
    if ( pthread_mutex_unlock(&__hlt_globals()->synced_fiber_pool_lock) != 0 )
        fatal_error("cannot unlock mutex");

    hlt_pthread_setcancelstate(i, NULL);
}

// Internal version that really creates a fiber (vs. the external version
// that might recycle a previously created on from a fiber pool). Note that
// this function does not intialize the "run" and "cookie" fields.
static hlt_fiber* __hlt_fiber_create(hlt_execution_context* ctx)
{
    hlt_fiber* fiber = (hlt_fiber*)hlt_malloc(sizeof(hlt_fiber));

    if ( getcontext(&fiber->uctx) < 0 ) {
        fprintf(stderr, "getcontext failed in __hlt_fiber_create\n");
        abort();
    }

    fiber->state = INIT;
    fiber->run = 0;
    fiber->cookie = 0;
    fiber->context = ctx;
    fiber->uctx.uc_link = 0;
    fiber->uctx.uc_stack.ss_size = hlt_config_get()->fiber_stack_size;
    fiber->uctx.uc_stack.ss_sp = hlt_stack_alloc(fiber->uctx.uc_stack.ss_size);
    fiber->uctx.uc_stack.ss_flags = 0;
    fiber->next = 0;

    // Magic from from libtask/task.c to turn the pointer into two words.
    unsigned long z = (unsigned long)fiber;
    unsigned int y = z;
    z >>= 16;
    unsigned int x = (z >> 16);

    makecontext(&fiber->uctx, (void (*)())_fiber_trampoline, 2, y, x);

    return fiber;
}

// Internal version that really deletes a fiber (vs. the external version
// that might put the fiber back into a pool to recycle later).
static void __hlt_fiber_delete(hlt_fiber* fiber)
{
    assert(fiber->state != RUNNING);

    hlt_stack_free(fiber->uctx.uc_stack.ss_sp, fiber->uctx.uc_stack.ss_size);
    hlt_free(fiber);
}

__hlt_fiber_pool* __hlt_fiber_pool_new()
{
    __hlt_fiber_pool* pool = hlt_malloc(sizeof(__hlt_fiber_pool));
    pool->head = 0;
    pool->size = 0;
    return pool;
}

void __hlt_fiber_pool_delete(__hlt_fiber_pool* pool)
{
    while ( pool->head ) {
        hlt_fiber* fiber = pool->head;
        pool->head = pool->head->next;
        __hlt_fiber_delete(fiber);
    }

    hlt_free(pool);
}

hlt_fiber* hlt_fiber_create(hlt_fiber_func func, hlt_execution_context* fctx, void* p,
                            hlt_execution_context* ctx)
{
    assert(ctx);

    // If there's a fiber available in the local pool, use that. Otherwise
    // check the global. Otherwise, create one.

    __hlt_fiber_pool* fiber_pool = ctx->worker ? ctx->worker->fiber_pool : ctx->fiber_pool;

    assert(fiber_pool);

    hlt_fiber* fiber = 0;

    if ( ! fiber_pool->head && hlt_is_multi_threaded() ) {
        __hlt_fiber_pool* global_pool = __hlt_globals()->synced_fiber_pool;

        // We do this without locking first, should be fine to encounter a
        // race.
        if ( global_pool->size ) {
            int s = 0;
            acqire_lock(&s);

            int n = hlt_config_get()->fiber_max_pool_size / 5; // 20%

            while ( global_pool->head && n-- ) {
                fiber = global_pool->head;

                global_pool->head = fiber->next;
                --global_pool->size;

                fiber->next = fiber_pool->head;
                fiber_pool->head = fiber;
                ++fiber_pool->size;
            }

            // fprintf(stderr, "vid %lu took %lu from global, that now at %lu\n", ctx->vid,
            // hlt_config_get()->fiber_max_pool_size / 10, global_pool->size);

            release_lock(s);
        }
    }

    if ( fiber_pool->head ) {
        fiber = fiber_pool->head;
        fiber_pool->head = fiber_pool->head->next;
        --fiber_pool->size;
        fiber->next = 0;
        assert(fiber->state == IDLE);
    }

    else
        fiber = __hlt_fiber_create(fctx);

    fiber->run = func;
    fiber->context = fctx;
    fiber->cookie = p;

    return fiber;
}

void hlt_fiber_delete(hlt_fiber* fiber, hlt_execution_context* ctx)
{
    assert(! fiber->next);

    if ( ! ctx ) {
        __hlt_fiber_delete(fiber);
        return;
    }

    __hlt_fiber_pool* fiber_pool = ctx->worker ? ctx->worker->fiber_pool : ctx->fiber_pool;

    // Return the fiber to the local pool as long as we haven't reached us
    // maximum pool size yet. If we have, return the pool to the global pool first.

    if ( fiber_pool->size >= hlt_config_get()->fiber_max_pool_size ) {
        if ( hlt_is_multi_threaded() ) {
            __hlt_fiber_pool* global_pool = __hlt_globals()->synced_fiber_pool;

            // We do this without locking, should be fine to encounter a
            // race.
            if ( global_pool->size <= 10 * hlt_config_get()->fiber_max_pool_size ) {
                int s = 0;
                acqire_lock(&s);

                // Check again.
                if ( global_pool->size <= 10 * hlt_config_get()->fiber_max_pool_size ) {
                    // fprintf(stderr, "vid %lu gives %lu to global, that then at %lu\n", ctx->vid,
                    // fiber_pool->size, global_pool->size + fiber_pool->size);

                    hlt_fiber* tail;

                    for ( tail = global_pool->head; tail && tail->next; tail = tail->next )
                        ;

                    if ( tail )
                        tail->next = fiber_pool->head;
                    else
                        global_pool->head = fiber_pool->head;

                    global_pool->size += fiber_pool->size;

                    fiber_pool->head = 0;
                    fiber_pool->size = 0;

                    release_lock(s);

                    goto return_to_local;
                }

                release_lock(s);
            }
        }

        // Local and global have reached their size, just return.
        __hlt_fiber_delete(fiber);
        return;
    }

return_to_local:

    hlt_stack_invalidate(fiber->uctx.uc_stack.ss_sp, fiber->uctx.uc_stack.ss_size);

    fiber->next = fiber_pool->head;
    fiber_pool->head = fiber;
    ++fiber_pool->size;
}

int8_t hlt_fiber_start(hlt_fiber* fiber, hlt_execution_context* ctx)
{
    int init = (fiber->state == INIT);

    __hlt_context_set_fiber(fiber->context, fiber);

    if ( ! _setjmp(fiber->parent) ) {
        fiber->state = RUNNING;

        if ( init )
            setcontext(&fiber->uctx);
        else
            _longjmp(fiber->fiber, 1);

        abort();
    }

    switch ( fiber->state ) {
    case YIELDED:
        __hlt_memory_safepoint(fiber->context, "fiber_start/yield");
        return 0;

    case IDLE:
        __hlt_memory_safepoint(fiber->context, "fiber_start/done");
        __hlt_context_set_fiber(fiber->context, 0);
        hlt_fiber_delete(fiber, ctx);
        return 1;

    default:
        abort();
    }
}


void hlt_fiber_yield(hlt_fiber* fiber)
{
    if ( ! _setjmp(fiber->fiber) ) {
        fiber->state = YIELDED;
        _longjmp(fiber->parent, 1);
    }
}

void hlt_fiber_return(hlt_fiber* fiber)
{
    __hlt_context_set_fiber(fiber->context, 0);
    _longjmp(fiber->trampoline, 1);
}

void hlt_fiber_set_result_ptr(hlt_fiber* fiber, void* p)
{
    fiber->result = p;
}

void* hlt_fiber_get_result_ptr(hlt_fiber* fiber)
{
    return fiber->result;
}

void* hlt_fiber_get_cookie(hlt_fiber* fiber)
{
    return fiber->cookie;
}

extern hlt_execution_context* hlt_fiber_context(hlt_fiber* fiber)
{
    return fiber->context;
}

void __hlt_fiber_init()
{
    if ( ! hlt_is_multi_threaded() ) {
        __hlt_globals()->synced_fiber_pool = 0;
        return;
    }

    if ( pthread_mutex_init(&__hlt_globals()->synced_fiber_pool_lock, 0) != 0 )
        fatal_error("cannot init mutex");

    __hlt_globals()->synced_fiber_pool = __hlt_fiber_pool_new();
}

void __hlt_fiber_done()
{
    if ( ! hlt_is_multi_threaded() )
        return;

    __hlt_fiber_pool_delete(__hlt_globals()->synced_fiber_pool);

    if ( hlt_is_multi_threaded() &&
         pthread_mutex_destroy(&__hlt_globals()->synced_fiber_pool_lock) != 0 )
        fatal_error("cannot destroy mutex");
}
