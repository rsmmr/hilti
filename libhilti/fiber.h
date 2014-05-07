
#ifndef LIBHILTI_FIBER_H
#define LIBHILTI_FIBER_H

#include <stdint.h>
#include "types.h"

struct __hlt_execution_context;

/// A fiber instance. This essentially capture the current execution state
/// and allow resumption at a later time.
typedef struct __hlt_fiber hlt_fiber;

typedef void (*hlt_fiber_func)(hlt_fiber* fiber, void* p);

/// Creates a new fiber instance.
///
/// func: The function to run inside the fiber. It receives two arguments:
/// the fiber and the pointer passed to hlt_fiber_create.
///
/// fctx: The context the fiber will run with.
///
/// p: A pointer that will be passed to the fiber's function.
///
/// ctx: The current context.
///
/// Returns: The new fiber.
extern hlt_fiber* hlt_fiber_create(hlt_fiber_func func, struct __hlt_execution_context* fctx, void* p, struct __hlt_execution_context* ctx);

/// Destroys a fiber. This release the stack, but note that it doesn't free
/// any pointers the stack may still hold. That must already have been done
/// to avoid leaks.
///
/// fiber: The fiber to destroy.
///
/// ctx: The current execution context.
extern void hlt_fiber_delete(hlt_fiber* fiber, hlt_execution_context* ctx);

/// Starts (or resumes) processing inside a fiber. If the fiber is freshly
/// created with hlt_fiber_create, processing will be kicked off; if it has
/// yielded via hlt_fiver_yield(), processing will continue where it left
/// off. This function returns only when the fiber finished or yielded. If
/// finished, the fiber will be deleted and must not be used anymore. 
///
/// fiber: The fiber
///
/// Returns: 1: fiber has finished. 0: fiber has yielded.
extern int8_t hlt_fiber_start(hlt_fiber* fiber, hlt_execution_context* ctx);

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

/// Returns the cookie value passed in when creating the fiber.
///
/// Returns: The cookie.
extern void* hlt_fiber_get_cookie(hlt_fiber* fiber);

/// Returns the execution context the fibers is running in.
///
/// Returns: The context.
extern struct __hlt_execution_context* hlt_fiber_context(hlt_fiber* fiber);

/// Internal functin to create a new, initially empty pool of available
/// fibers.
extern __hlt_fiber_pool* __hlt_fiber_pool_new();

/// Internal function to delete a pool of available fibers.
extern void __hlt_fiber_pool_delete(__hlt_fiber_pool* pool);

void __hlt_fiber_init();
void __hlt_files_done();


#endif
