// @TEST-IGNORE

#include <stdio.h>
#include <libhilti.h>

#include "callable.hlt.h"

int main()
{
    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    ///

    hlt_callable* c = foo_create_void(&excpt, ctx);
    HLT_CALLABLE_RUN(c, 0, Foo_MyCallableVoid, &excpt, ctx);

    ///
    fprintf(stdout, "===\n");

    c = foo_create_result(&excpt, ctx);

    hlt_bytes* b = hlt_bytes_new_from_data_copy((const int8_t*)"ABCDE", 5, &excpt, ctx);
    int64_t i = 10000;

    hlt_string s;
    HLT_CALLABLE_RUN(c, &s, Foo_MyCallableResult, &b, &i, &excpt, ctx);

    fprintf(stdout, "C: ");
    hlt_string_print(stdout, s, true, &excpt, ctx);

    //

    return 0;
}
