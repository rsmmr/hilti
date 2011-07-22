// $Id$

#ifndef HLT_CONTEXT_H
#define HLT_CONTEXT_H

#include "continuation.h"
#include "context.h"
#include "profiler.h"

struct hlt_worker_thread;

/// A per-thread execution context. This is however just the common header of
/// all contexts; in memory, the header will be followed with the set of
/// thread-local variables.
typedef struct hlt_execution_context {
    hlt_vthread_id vid;               /// The ID of the virtual thread this context belongs to. HLT_VID_MAIN for the main thread.
    struct hlt_worker_thread* worker; /// The worker thread this virtual thread is mapped to. NULL for the main thread.
    hlt_continuation* yield;          /// A continuation to call when a ``yield`` statement is executed.
    hlt_continuation* resume;         /// A continuation to call when resuming after a ``yield`` statement. Is set by the ``yield``.

    /// We keep an array of callable registered for execution but not
    /// processed yet.
    uint64_t       calls_num;     /// Number of callables in array.
    uint64_t       calls_alloced; /// Number of slots allocated.
    uint64_t       calls_first;   /// Index of first non-yet processed element.
    hlt_callable** calls;         /// First element of array, or 0 if empty.

    hlt_profiling_state* pstate;  /// State for ongoing profiling, or 0 if none.

    // TODO; We should not compile this in in non-debug mode.
    uint64_t debug_indent;        /// Current indent level for debug messages.

    void* tcontext; /// The current threading context, per the module's "context" definition; NULL if not set.
} hlt_execution_context;

typedef void yield_func(void* frame, void* eoss, hlt_execution_context* ctx);

/// Creates a new execution context.
///
/// vid: The ID of the virtual thread the context will belong to.
/// ~~HLT_VID_MAIN for the main (non-worker) thread.
///
/// yield: A function to return control to when a ``yield`` statement is
/// executed. The function must have the standard HILTI signature, i.e.,
/// ``void f(void* frame, void* eoss, hlt_execution_context* ctx``. However,
/// the two former fields will always be 0 when the function is called. The
/// ``ctx`` will always be a pointer to the newly allocated context.
///
/// Returns: The new context.
hlt_execution_context* hlt_execution_context_new(hlt_vthread_id vid, yield_func* func);

#endif
