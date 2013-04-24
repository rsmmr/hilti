/*

  We don't integrate this into the test-suite, it's for manual benchmarking.

  @TEST-IGNORE
  @TEST-EXEC:  hilti-build -v %INPUT -o a.out
*/

#include <libhilti.h>
#include <assert.h>
#include <sys/time.h>

#include <libhilti.h>

double current_time()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (double)(tv.tv_sec) + (double)(tv.tv_usec) / 1e6;
}

void fiber_func_return(hlt_fiber* fiber, void* p)
{
    hlt_fiber_return(fiber);
}

void fiber_func_yield(hlt_fiber* fiber, void* p)
{
    int* cnt = (int *)p;

    while ( --*cnt )
        hlt_fiber_yield(fiber);

    hlt_fiber_return(fiber);
}

int main(int argc, char** argv)
{
    hlt_init();

    hlt_fiber* fiber = 0;

    int rounds = 10000000;
    int cnt = rounds;
    double start = current_time();

    fiber = hlt_fiber_create(fiber_func_yield, hlt_global_execution_context(), (void*)&cnt);

    while ( 1 ) {
        int r = hlt_fiber_start(fiber);
        if ( r == 1 )
            break;
    }

    hlt_fiber_delete(fiber);

    double delta = current_time() - start;
    double rate = rounds / delta;

    fprintf(stderr, "start/yield: %.2fs => %.2fs rounds/sec\n", delta, rate);

    //////////////////////

    rounds = 100;
    start = current_time();

    for ( int i = 0; i < rounds; i++ ) {
        fiber = hlt_fiber_create(fiber_func_return, hlt_global_execution_context(), (void*)0x1234567890);
        int r = hlt_fiber_start(fiber);
        assert(r == 1);
        hlt_fiber_delete(fiber);
    }

    delta = current_time() - start;
    rate = rounds / delta;

    fprintf(stderr, "create/start/return/delete: %.2fs => %.2f rounds/sec\n", delta, rounds / delta);

    //hlt_done();

    return 0;
}

