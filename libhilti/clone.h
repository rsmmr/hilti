/// Support for deep-copying HILTI values. 

#ifndef LIBHILTI_CLONE_H
#define LIBHILTI_CLONE_H

#include "types.h"
#include "hutil.h"

struct __hlt_clone_state {
    int32_t type;            // __HLT_CLONE_*
    int32_t level;           // Current recursion level.
    hlt_vthread_id vid;      // Thread hint.
    __hlt_pointer_map objs;  // Cache of objects seen already.
};

/// Deep-copies a HILTI value for use within the same thread.
///
/// dstp: A pointer to where the cloned version is to be stored. For garbage
/// collected types, this is where a *pointer* to the cloned object will be
/// stored. For all other types, it's where the object itself will be placed,
/// and it must provide sufficient space for at least as many bytes as \c
/// ti->size specifies. The new object will be created with reference count
/// +1, with ownership passed to the caller.
///
/// ti: The type information for the object to be cloned.
///
/// src: The source object to be cloned. For garbage collected types this can
/// be null, in which case the cloned value will be so too.
///
/// excpt: &
/// ctx: &
extern void hlt_clone_deep(void* dstp, const hlt_type_info* ti, const void* srcp, hlt_exception** excpt, hlt_execution_context* ctx);

/// Shallow-copies a HILTI value for use within the same thread.
///
/// dstp: A pointer to where the cloned version is to be stored. For garbage
/// collected types, this is where a *pointer* to the cloned object will be
/// stored. For all other types, it's where the object itself will be placed,
/// and it must provide sufficient space for at least as many bytes as \c
/// ti->size specifies. The new object will be created with reference count
/// +1, with ownership passed to the caller.
///
/// ti: The type information for the object to be cloned.
///
/// src: The source object to be cloned. For garbage collected types this can
/// be null, in which case the cloned value will be so too.
///
/// excpt: &
/// ctx: & 
extern void hlt_clone_shallow(void* dstp, const hlt_type_info* ti, const void* srcp, hlt_exception** excpt, hlt_execution_context* ctx);

/// Clones a HILTI value for use within a specific thread. If the specific
/// thread is the current one, this will do the same as deep copy by
/// hlt_clone(). If not, the cloning may be adjusted by types to accomodate
/// use (only) in a different thread. This will never make a shallow copy.
///
/// dstp: A pointer to where the cloned version is to be stored. For garbage
/// collected types, this is where a *pointer* to the cloned object will be
/// stored. For all other types, it's where the object itself will be placed,
/// and it must provide sufficient space for at least as many bytes as \c
/// ti->size specifies. The new object will be created with reference count
/// +1, with ownership passed to the caller.
///
/// ti: The type information for the object to be cloned.
///
/// srcp: The source object to be cloned. For garbage collected types this can
/// be null, in which case the cloned value will be so too.
///
/// vid: The thread which thread will take ownership of the cloned object.
///
/// excpt: &
/// ctx: & 
extern void hlt_clone_for_thread(void* dstp, const hlt_type_info* ti, const void* srcp, hlt_vthread_id vid, hlt_exception** excpt, hlt_execution_context* ctx);

/// Internal version of the clone method that must be used by type-specific
/// clone implementations when cloning values recursively that they contain.
/// This must be called independent of what clone type is in use (deep or
/// shallow); it will do the right thing in either case. It also deals
/// correctly with reference cycles encountered during the recursion,
/// recreating them in the clone.
///
/// Arguments are the same as with hlt_clone() except for the clone state
/// parameter, which must be passed through from the RTTI clone_init()
/// function.
extern void __hlt_clone(void* dstp, const hlt_type_info* ti, const void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx);

typedef void (*__hlt_init_in_thread_callback)(const hlt_type_info* ti, void* dstp, hlt_exception** excpt, hlt_execution_context* ctx);

/// Internal helper for clone implementation to delay instance intialization
/// until executing in the target thread. The caller gives a callback that
/// will execute either immediately if we're already in the right thread, or
/// later if not.
extern void __hlt_clone_init_in_thread(__hlt_init_in_thread_callback cb, const hlt_type_info* ti, void* dstp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
