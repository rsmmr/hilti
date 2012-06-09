///
/// All thread-global state is stored in execution contexts.

#ifndef HLT_CALLABLE_H
#define HLT_CALLABLE_H

#include "memory_.h"
#include "types.h"
#include "rtti.h"

/// A callable, i.e., a bound function call.
typedef struct hlt_callable hlt_callable;

/// Executes a bound callable. Running a callable from C via the function
/// always discards any return value.
extern void hlt_callable_run(hlt_callable* c, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
