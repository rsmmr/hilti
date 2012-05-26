
#ifndef HILTI_FIBER_H
#define HILTI_FIBER_H

#include <stdint.h>

struct __hlt_execution_context;

/// A fiber instance. This essentially capture the current execution state
/// and allow resumption at a later time.
typedef struct __hlt_fiber hlt_fiber;

typedef void (*hlt_fiber_func)(hlt_fiber* fiber, void* p);

/// Creates a new fiber instance.
///
/// func: The function to run inside the fiber. It receives the pointer
/// passed to hlt_fiber_create.
///
/// ctx: The context the fiber will run with.
///
/// p: A pointer that will be passed to the fiber's function.
///
/// Returns: The new fiber.
extern hlt_fiber* hlt_fiber_create(hlt_fiber_func func, struct __hlt_execution_context* ctx, void* p);

/// Destroys a fiber. This release the stack, but note that it doesn't free
/// any pointers the stack may still hold. That must already have been done
/// to avoid leaks.
///
/// fiber: The fiber to destroy.
extern void hlt_fiber_delete(hlt_fiber* fiber);

/// Starts processing inside a fiber. This function will return only when the
/// fiber finished or yielded.
///
/// fiber: The fiber
///
/// Returns: 1: fiber has finished. 0: fiber has yielded.
extern int8_t hlt_fiber_start(hlt_fiber* fiber);

/// Exits processing regularly from within a fiber, like a top-level return
/// statement.
///
/// fiber: The fiber
extern void hlt_fiber_return(hlt_fiber* fiber);

/// Yields control back to the caller from inside a fiber. The fiber's
/// processing can later be resumed with hlt_fiber_resume().
///
/// fiber: The fiber
extern void hlt_fiber_yield(hlt_fiber* fiber);

/// Resumes a fiber where hlt_fiber_yield() left off. Behaviour is undefined
/// if yield hasn't been called yet since the most recent hlt_fiber_start().
///
/// fiber: The fiber
///
/// Returns: 1: fiber has finished. 0: fiber has yielded.
extern int8_t hlt_fiber_resume(hlt_fiber* fiber);

/// Saves a pointer in the fiber indicating where a return value should be stored.
///
/// ptr: The pointer.
///
extern void hlt_fiber_set_result_ptr(hlt_fiber* fiber, void* p);

/// Returns a previously saved pointer from the fiber indicating where a
/// return value should be stored.
///
/// Returns: The pointer.
extern void* hlt_fiber_get_result_ptr(hlt_fiber* fiber);

/// Returns the execution context the fibers is running in.
///
/// Returns: The context.
extern struct __hlt_execution_context* hlt_fiber_context(hlt_fiber* fiber);

#endif
