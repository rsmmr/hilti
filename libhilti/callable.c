
#include <stdio.h>

#include "callable.h"
#include "context.h"

// Static part of callable description. This is function-specific but not
// argument specific.
//
// Adapt hlt.callable.func when making changes here!
typedef struct {
    void* run; // Run function that may return a value. Cannot be called from C.
    void (*run_no_result)(hlt_callable* callable, hlt_execution_context* ctx); // Run function not returning a value.
    void (*dtor)(hlt_callable* callable); // Dtor function.
} __hlt_callable_func;

// Definition of a callable.
//
// Adapt hlt.callable when making changes here!
struct hlt_callable {
    __hlt_gchdr __gch;                // Header for garbage collection.
    __hlt_callable_func* __func;      // Pointer to the set of function for the callable.
                                      // Arguments follow here in memory.
};

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

