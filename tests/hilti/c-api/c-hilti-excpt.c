
// @TEST-IGNORE

#include <stdio.h>
#include <libhilti.h>

extern void foo_foo(hlt_exception**, hlt_execution_context*);

int main()
{
    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    foo_foo(&excpt, ctx);

    if ( excpt )
        {
        hlt_exception_print(excpt, ctx);
        GC_DTOR(excpt, hlt_exception);
        }

    else
        fprintf(stderr, "no exception\n");
}

