/*

@TEST-EXEC:  hilti-build -v %INPUT -o a.out
@TEST-EXEC:  ./a.out >output 2>&1
@TEST-EXEC:  btest-diff output

*/

#include <libhilti.h>
#include <assert.h>

#include <libhilti.h>

void fiber_yielded(hlt_fiber* f)
{
    switch ( hlt_fiber_start(f) ) {
     case 0:
        fprintf(stderr, "Fiber yielded again\n");
        fiber_yielded(f);
        break;
     case 1:
        fprintf(stderr, "Fiber finished\n");
        break;

     default:
        assert(0);
    }
}

void fiber_func(hlt_fiber* fiber, void* p)
{
    fprintf(stderr, "In fiber (p=%p)\n", p);
    hlt_fiber_yield(fiber);
    fprintf(stderr, "Back in fiber\n");
    hlt_fiber_yield(fiber);
    fprintf(stderr, "Done with fiber\n");
    hlt_fiber_return(fiber);
    fprintf(stderr, "Cannot be reached A\n");
}

int main(int argc, char** argv)
{
    hlt_init();

    hlt_fiber* fiber = hlt_fiber_create(fiber_func, hlt_global_execution_context(), (void*)0x1234567890);

    fprintf(stderr, "Init\n");

    switch ( hlt_fiber_start(fiber) ) {
     case 0:
        // Fiber yielded.
        fprintf(stderr, "Fiber yielded\n");
        fiber_yielded(fiber);
        break;

     case 1:
        // Fiber finished.
        fprintf(stderr, "Finished, but should not be reached\n");
        break;

     default:
        assert(0); // Cannot be reached.
    }

    hlt_done();

    return 0;
}

