///
/// All thread-global state is stored in execution contexts.

#ifndef HLT_CALLABLE_H
#define HLT_CALLABLE_H

#include "memory_.h"
#include "types.h"
#include "rtti.h"
#include "threading.h"

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
struct __hlt_callable {
    __hlt_gchdr __gch;                     // Header for garbage collection.
    __hlt_callable_func* __func;           // Pointer to the set of function for the callable.
    ;                                      // Arguments follow here in memory.
};


/// Executes a bound callable. Running a callable from C via the function
/// always discards any return value.
extern void hlt_callable_run(hlt_callable* c, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
