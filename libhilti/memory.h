///
/// Memory management.
///

#ifndef HILTI_MEMORY_H
#define HILTI_MEMORY_H

#include "types.h"

#include <stdint.h>
#include <stdlib.h>

/// Common header of all managed memory objects.
///
/// If you change something here, also adapt ``hlt.gcdhr`` in ``libhilti.ll``.
typedef struct {
    uint64_t ref_cnt;  /// The number of references to the object currently retained.
} __hlt_gchdr;

/// Allocates a new memory managed object of the given type and size. The
/// size given must include the #__gc_hdr header. The objects reference count
/// will be initialized to one. The function always returns a valid pointer
/// to a new object. If the function can't allocate sufficient memory, it
/// will terminate program execution directly and not return.
///
/// ti: Type information corresponding to the type being allocated.
///
/// size: The number of bytes to allocate.
///
/// Returns: A pointer to the new memory chunk. This will never be null.
extern void* hlt_object_new(const hlt_type_info* ti, size_t size);

/// Increaes a memory managed object's reference count by one.
///
/// obj: A pointer to the object. It must start with a ``hlt_gchdr` instance
/// and currently have a reference count larger than zero.
///
/// ti: Type information corresponding to the type of #obj.
///
extern void hlt_object_ref(const hlt_type_info* ti, void* obj);

/// Unrefs a memory object. The object is assumed to start with a
/// ``hlt_hcgdr``. The current reference count must be larger than zero. If
/// the reference count goes to zero by this operation, the object will be
/// destroyed (though we leave it undefined *when* that will happen. It could
/// be later.) To destroy an object, it first calls the function *dtor*,
/// which should recursively unref all managed subcomponent. It will then
/// release the memory and from then on the passed pointer will be invalid.
///
/// obj: The object to unref. Can be null, in which case the function will
/// not do anything.
///
/// ti: Type information corresponding to the type of #obj.
extern void hlt_object_unref(const hlt_type_info* ti, void* obj);

/// Allocates an unmanaged memory chunk of the given size. This operates
/// pretty much like malloc but it will always return a valid address. If it
/// can't allocate sufficient memory, the function will terminate the current
/// process directly and not return. The allocated memory must be freed with
/// ``hlt_free``.
///
/// size: The number of bytes to allocate.
///
/// Returns: A pointer to the new memory chunk. This will never be null.
extern void* hlt_malloc(size_t size);

/// Allocates a number of unmanaged memory chunks of the given size. This
/// operates pretty much like calloc but it will always return a valid
/// address. If it can't allocate sufficient memory, the function will
/// terminate the current process directly and not return. The allocated
/// memory must be freed with ``hlt_free``.
///
/// size: The number of bytes to allocate.
///
/// Returns: A pointer to the new memory chunk. This will never be null.
extern void* hlt_calloc(size_t count, size_t size);

/// Frees an unmanaged memory chunk formerly allocated with ``hlt_malloc``.
/// This operates pretty much like the standard ``free``. 
///
/// memory: The memory chunk to free. Can be null, in which case the function
/// will not do anything.
extern void hlt_free(void *memory);

#endif
