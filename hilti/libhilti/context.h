// $Id$
// 
/// Global execution context. An execution context is a collection of
/// attributes globally available in each execution unit. In a
/// single-threaded environment, there's only one execution unit and thus
/// also only one global execution object. In a multi-threaded environments,
/// each thread has it's own execution context and the access functions
/// transparently return the current thread's context. 
/// 
/// Note: This data is not really used at all at the moment, but will in the
/// future.

#ifndef HLT_CONTEXT_H
#define HLT_CONTEXT_H

typedef struct {
    // Pointer to global variables (global relative to this context).
    // FIXME: Currently unused because globals aren't implemented yet. 
    void* globals; 
} hlt_execution_context;

// The global context which is used in single-threaded setups.
hlt_execution_context __hlt_global_execution_context;

void __hlt_execution_context_init(hlt_execution_context* ctx);
void __hlt_execution_context_done(hlt_execution_context* ctx);

/// Returns the current execution context. The context is relative to the
/// current execution unit (i.e., either the global context in a
/// single-threaded setup, or the thread-local context in a multi-threaded
/// setup.)
/// 
/// TODO: The multi-threaded case has not been tested ...
hlt_execution_context* hlt_current_execution_context();

#endif HLT_CONTEXT_H
