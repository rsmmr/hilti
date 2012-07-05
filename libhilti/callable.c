
#include <stdio.h>

#include "callable.h"
#include "context.h"

void hlt_callable_dtor(hlt_type_info* ti, hlt_callable* c)
{
    if ( c->__func->dtor )
        (*c->__func->dtor)(c);
}

void hlt_callable_run(hlt_callable* c, hlt_exception** excpt, hlt_execution_context* ctx)
{
    (*c->__func->run_no_result)(c, ctx);

    // Need to transfer exception information over.
    *excpt = __hlt_context_get_exception(ctx);
}

