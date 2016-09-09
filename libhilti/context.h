///
/// All thread-global state is stored in execution contexts.

#ifndef HLT_CONTEXT_H
#define HLT_CONTEXT_H

#include "fiber.h"
#include "memory_.h"
#include "rtti.h"
#include "types.h"

struct __hlt_worker_thread;

/// A per-thread execution context. This is however just the common header of
/// all contexts. In memory, the header will be followed with the set of
/// thread-global variables.
///
/// When changing this, adapt ``hlt.execution_context`` in ``libhilti.ll``
/// and ``codegen::hlt::ExecutionContext::Globals``.
struct __hlt_execution_context {
    __hlt_gchdr __gch;    /// Header for garbage collection.
    hlt_vthread_id vid;   /// The ID of the virtual thread this context belongs to. HLT_VID_MAIN for
                          /// the main thread.
    hlt_exception* excpt; /// The currently raised exception, or 0 if none.
    hlt_fiber* fiber; /// The current fiber to use for executing code inside this context. If set
                      /// when the context is destroyed, it will be deleted.
    __hlt_fiber_pool* fiber_pool; /// The pool of available fiber objects for the main thread. //
                                  /// TODO: Move somewhere else?
    struct __hlt_worker_thread*
        worker; /// The worker thread this virtual thread is mapped to. NULL for the main thread.
    void* tcontext; /// The current threading context, per the module's "context" definition; NULL
                    /// if not set. This is ref counted.
    hlt_type_info* tcontext_type;          /// The type of the current threading context.
    __hlt_thread_mgr_blockable* blockable; /// A blockable set to go along with the next yield.
    hlt_timer_mgr* tmgr;                   /// The context's timer manager.
    __hlt_memory_nullbuffer* nullbuffer;   /// Null-buffer for delayed reference counting.

    // TODO: We should not compile this in non-profiling mode.
    __hlt_profiler_state* pstate; /// State for ongoing profiling, or 0 if none.

    // TODO; We should not compile this in non-debug mode.
    uint64_t debug_indent; /// Current indent level for debug messages.

    void* globals; // The globals are starting right here (at the address of the field, the pointer
                   // content is ignored.)
};

/// Creates a new execution context.
///
/// Note that execution contexts are not reference counted.
///
/// vid: The ID of the virtual thread the context will belong to.
/// ~~HLT_VID_MAIN for the main (non-worker) thread.
///
/// run_module_init: Whether to run the modules' init functions.
///
/// Returns: The new context at +1.
extern hlt_execution_context* __hlt_execution_context_new_ref(hlt_vthread_id vid,
                                                              int8_t run_module_init);

/// Deletes an execution context.
///
/// ctx: The context to delete.
void hlt_execution_context_delete(hlt_execution_context* ctx);

/// Sets the exception field in an execution context. The object must be
/// passed at +1.
///
/// excpt: The exception.
///
/// ctx: The context.
extern void __hlt_context_set_exception(hlt_execution_context* ctx, hlt_exception* excpt);

/// Returns the exception field from an execution context.
///
/// ctx: The context.
///
/// Returns: The exception.
extern hlt_exception* __hlt_context_get_exception(hlt_execution_context* ctx);

/// Clears the exception field in an execution context.
///
/// ctx: The context.
extern void __hlt_context_clear_exception(hlt_execution_context* ctx);

/// Returns the fiber field from an execution context.
///
/// ctx: The context.
///
/// Returns: The fiber.
extern hlt_fiber* __hlt_context_get_fiber(hlt_execution_context* ctx);

/// Returns the virtual thread ID from an execution context.
///
/// ctx: The context.
///
/// Returns: The vid.
extern hlt_vthread_id __hlt_context_get_vid(hlt_execution_context* ctx);

/// Sets the fiber field in an execution context.
///
/// ctx: The context.
///
/// fiber: The fiber.
extern void __hlt_context_set_fiber(hlt_execution_context* ctx, hlt_fiber* fiber);

/// Returns the thread context field from an execution context.
///
/// ctx: The context.
///
/// Returns: The thread context.
extern void* __hlt_context_get_thread_context(hlt_execution_context* ctx);

/// Sets the thread context field in an execution context.
///
/// ctx: The context.
///
/// type: The type of the thread context.
///
/// tctx: The thread context.
extern void __hlt_context_set_thread_context(hlt_execution_context* ctx, hlt_type_info* type,
                                             void* tctx);

/// Sets the blockable field in an execution context.
///
/// ctx: The context.
///
/// b: The blockable to set the field to.
extern void __hlt_context_set_blockable(hlt_execution_context* ctx, __hlt_thread_mgr_blockable* b);

#endif
