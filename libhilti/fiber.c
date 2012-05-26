//
// This follows roughly the idea from
// http://www.1024cores.net/home/lock-free-algorithms/tricks/fibers.
//

#include <stdio.h>
#include <setjmp.h>

#include "fiber.h"
#include "memory.h"

#include "3rdparty/libtask/taskimpl.h"

enum __hlt_fiber_state { INIT=1, RUNNING=2, FINISHED=3, YIELDED=4 };

struct __hlt_fiber {
    ucontext_t uctx;
    jmp_buf fiber;
    jmp_buf parent;
    void* cookie;
    void* result;
    hlt_fiber_func run;
    enum __hlt_fiber_state state;
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

hlt_fiber* hlt_fiber_create(int stack)
{
    hlt_fiber* fiber = (hlt_fiber*) hlt_malloc(sizeof(hlt_fiber));

    if ( getcontext(&fiber->uctx) < 0 ) {
        fprintf(stderr, "getcontext failed in hlt_fiber_create\n");
        abort();
    }

    fiber->state = INIT;
    fiber->uctx.uc_link = 0;
    fiber->uctx.uc_stack.ss_sp = hlt_malloc(stack);
    fiber->uctx.uc_stack.ss_size = stack;
    fiber->uctx.uc_stack.ss_flags = 0;

    return fiber;
}

void hlt_fiber_delete(hlt_fiber* fiber)
{
    hlt_free(fiber->uctx.uc_stack.ss_sp);
    hlt_free(fiber);
}

int8_t hlt_fiber_start(hlt_fiber* fiber, hlt_fiber_func func, void* p)
{
    fiber->cookie = p;
    fiber->run = func;

    // Magic from from libtask/task.c to turn the pointer into two words.
    unsigned long z = (unsigned long)fiber;
    unsigned int y = z;
    z >>= 16;
    unsigned int x = (z >> 16);

    makecontext(&fiber->uctx, (void (*)())_fiber_trampoline, 2, y, x);

    if ( ! _setjmp(fiber->parent) ) {
        fiber->state = RUNNING;
        setcontext(&fiber->uctx);
        abort();
    }

    switch ( fiber->state ) {
     case FINISHED:
        return 1;

     case YIELDED:
        return 0;

     default:
        abort();
    }
}

int8_t hlt_fiber_resume(hlt_fiber* fiber)
{
    if ( ! _setjmp(fiber->parent) ) {
        fiber->state = RUNNING;
        _longjmp(fiber->fiber, 1);
        abort();
    }

    switch ( fiber->state ) {
     case FINISHED:
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
