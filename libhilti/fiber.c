//
// This follows roughly the idea from
// http://www.1024cores.net/home/lock-free-algorithms/tricks/fibers.
//

#include <stdio.h>
#include <setjmp.h>

#include "fiber.h"
#include "config.h"
#include "memory_.h"
#include "context.h"

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
    uint64_t stack[]; // Begin of stack. We use an uint64_t to make sure it's well aligned.
};

struct __hlt_fiber_pool {
    hlt_fiber* head;
    size_t size;
};

static void __hlt_fiber_yield(hlt_fiber* fiber, enum __hlt_fiber_state state);
static void __hlt_fiber_return(hlt_fiber* fiber, enum __hlt_fiber_state state);

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

// Internal version that really creates a fiber (vs. the external version
// that might recycle a previously created on from a fiber pool). Note that
// this function does not intialize the "run" and "cookie" fields.
static hlt_fiber* __hlt_fiber_create(hlt_execution_context* ctx)
{
    size_t stack_size = hlt_config_get()->fiber_stack_size;

    hlt_fiber* fiber = (hlt_fiber*) hlt_malloc(sizeof(hlt_fiber) + stack_size);

    if ( getcontext(&fiber->uctx) < 0 ) {
        fprintf(stderr, "getcontext failed in __hlt_fiber_create\n");
        abort();
    }

    fiber->state = INIT;
    fiber->run = 0;
    fiber->cookie = 0;
    fiber->context = ctx;
    fiber->uctx.uc_link = 0;
    fiber->uctx.uc_stack.ss_size = stack_size;
    fiber->uctx.uc_stack.ss_sp = &fiber->stack;
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

hlt_fiber* hlt_fiber_create(hlt_fiber_func func, hlt_execution_context* ctx, void* p)
{
    // If there's a fiber available in the pool, use that. Otherwise create a
    // new one.

    hlt_fiber* fiber = 0;

    if ( ctx->fiber_pool->head ) {
        fiber = ctx->fiber_pool->head;
        ctx->fiber_pool->head = ctx->fiber_pool->head->next;
        --ctx->fiber_pool->size;
        fiber->next = 0;
        assert(fiber->state == IDLE);
    }

    else
        fiber = __hlt_fiber_create(ctx);

    fiber->run = func;
    fiber->cookie = p;

    return fiber;
}

void hlt_fiber_delete(hlt_fiber* fiber)
{
    assert(! fiber->next);

    // Return the fiber to the pool as long as we haven't reached us maximum
    // pool size yet.

    if ( fiber->context->fiber_pool->size >= hlt_config_get()->fiber_max_pool_size ) {
        __hlt_fiber_delete(fiber);
        return;
    }

    fiber->next = fiber->context->fiber_pool->head;
    fiber->context->fiber_pool->head = fiber;
    ++fiber->context->fiber_pool->size;
}

int8_t hlt_fiber_start(hlt_fiber* fiber)
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
        return 0;

     case IDLE:
        __hlt_context_set_fiber(fiber->context, 0);
        hlt_fiber_delete(fiber);
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
