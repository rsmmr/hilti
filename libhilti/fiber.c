//
// This follows roughly the idea from
// http://www.1024cores.net/home/lock-free-algorithms/tricks/fibers.
//

#include <stdio.h>
#include <setjmp.h>

#include "fiber.h"
#include "memory_.h"
#include "context.h"

#include "3rdparty/libtask/taskimpl.h"

// TODO: This should be customizable, however the we'll need separate free
// lists.
static const int STACK_SIZE = 10 * 1024 * 1024;

enum __hlt_fiber_state { INIT=1, RUNNING=2, FINISHED=3, YIELDED=4 };

struct __hlt_fiber {
    enum __hlt_fiber_state state;
    ucontext_t uctx;
    jmp_buf fiber;
    jmp_buf parent;
    void* cookie;
    void* result;
    hlt_execution_context* context;
    hlt_fiber_func run;
    char stack[STACK_SIZE];
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

    (*fiber->run)(fiber, fiber->cookie);

    hlt_fiber_return(fiber);
}

hlt_fiber* hlt_fiber_create(hlt_fiber_func func, hlt_execution_context* ctx, void* p)
{
    hlt_fiber* fiber = (hlt_fiber*) hlt_free_list_alloc(ctx->fiber_pool, sizeof(hlt_fiber));

    if ( getcontext(&fiber->uctx) < 0 ) {
        fprintf(stderr, "getcontext failed in hlt_fiber_create\n");
        abort();
    }

    fiber->state = INIT;
    fiber->run = func;
    fiber->cookie = p;
    fiber->context = ctx;
    fiber->uctx.uc_link = 0;
    fiber->uctx.uc_stack.ss_sp = &fiber->stack;
    fiber->uctx.uc_stack.ss_size = STACK_SIZE;
    fiber->uctx.uc_stack.ss_flags = 0;

    // Magic from from libtask/task.c to turn the pointer into two words.
    unsigned long z = (unsigned long)fiber;
    unsigned int y = z;
    z >>= 16;
    unsigned int x = (z >> 16);

    makecontext(&fiber->uctx, (void (*)())_fiber_trampoline, 2, y, x);

    return fiber;
}

void hlt_fiber_delete(hlt_fiber* fiber)
{
    hlt_free_list_free(fiber->context->fiber_pool, fiber, sizeof(hlt_fiber));
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
     case FINISHED:
        __hlt_context_set_fiber(fiber->context, 0);
        hlt_fiber_delete(fiber);
        return 1;

     case YIELDED:
        return 0;

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
    if ( ! _setjmp(fiber->fiber) ) {
        __hlt_context_set_fiber(fiber->context, 0);
        fiber->state = FINISHED;
        _longjmp(fiber->parent, 1);
    }
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
