// $Id$

#ifndef HLT_CONTEXT_H
#define HLT_CONTEXT_H

#if 0

#include "threading.h"

/// A per-thread execution context. This is however just the common header of
/// all contexts; in memory, the header will be followed with the set of
/// thread-local variables.
typedef struct {
    hlt_vthread_id vid;        /// The ID of the virtual thread this context belongs to. HLT_VID_MAIN for the main thread.
    hlt_worker_thread* worker; /// The worker thread this virtual thread is mapped to. NULL for the main thread. 
    hlt_continuation* yield;   /// A continuation to call when a ``yield`` statement is executed.
    hlt_continuation* resume;  /// A continuation to call when resuming after a ``yield`` statement. Is set by the ``yield``.
};

typedef yield_func(void* frame, void* eoss, hlt_execution_ctx* ctx);

/// Creates a new execution context. 
/// 
/// vid: The ID of the virtual thread the context will belong to.
/// ~~HLT_VID_MAIN for the main (non-worker) thread.  
/// 
/// yield: A function to return control to when a ``yield`` statement is
/// executed. The function must have the standard HILTI signature, i.e.,
/// ``void f(void* frame, void* eoss, hlt_execution_ctx* ctx``. However, the
/// two former fields will always be 0 when the function is called. The
/// ``ctx`` will always be a pointer to the newly allocated context. 
/// 
/// Returns: The new context. 
hlt_exceution_ctx* hlt_execution_context_new(hlt_vthread_id vid, yield_func* func);

#else
typedef int hlt_vthread_id;
struct hlt_execution_context;
#endif

#endif 
