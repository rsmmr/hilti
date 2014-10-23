
#include <stdio.h>

#include "callable.h"
#include "context.h"

void hlt_callable_dtor(hlt_type_info* ti, hlt_callable* c, hlt_execution_context* ctx)
{
    if ( c->__func->dtor )
        (*c->__func->dtor)(c, ctx);
}

void* hlt_callable_clone_alloc(const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_callable* src = *(hlt_callable**)srcp;
    return GC_NEW_CUSTOM_SIZE_GENERIC_REF(ti, src->__func->object_size, ctx);
}

void hlt_callable_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_callable* src = *(hlt_callable**)srcp;
    hlt_callable* dst = *(hlt_callable**)dstp;

    dst->__func = src->__func;

    if ( src->__func->clone_init )
        (*src->__func->clone_init)(dst, src, cstate, excpt, ctx);
}
