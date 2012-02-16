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

// Internal function to increaes a memory managed object's reference count by
// one. Not to be used directly from user code.
extern void __hlt_object_ref(const hlt_type_info* ti, void* obj);

// Internal function to decrease a memory managed object's reference count by
// one. Not to be used directly from user code.
extern void __hlt_object_unref(const hlt_type_info* ti, void* obj);

/// XXX
extern void __hlt_object_dtor(const hlt_type_info* ti, void* obj);

/// XXX
extern void __hlt_object_cctor(const hlt_type_info* ti, void* obj);

/// XXX
extern void* __hlt_object_new(const hlt_type_info* ti, size_t size);

/// XXX
#define GC_ASSIGN(obj, val, tag) \
   { \
   __hlt_object_dtor(hlt_type_info_##tag, &obj); \
   obj = val; \
   __hlt_object_cctor(hlt_type_info_##tag, &obj); \
   }

/// XXX
#define GC_NEW(tag) \
   __hlt_object_new(hlt_type_info_##tag, sizeof(tag))

/// XXX
#define GC_NEW_CUSTOM_SIZE(tag, size) \
   __hlt_object_new(hlt_type_info_##tag, size)

/// XXX
#define GC_DTOR(obj, tag) \
   { \
       __hlt_object_dtor(hlt_type_info_##tag, &obj); \
   }

/// XXX
#define GC_CCTOR(obj, tag) \
   { \
       __hlt_object_cctor(hlt_type_info_##tag, &obj); \
   }

/// XXX
#define GC_CLEAR(obj, tag) \
   { \
       __hlt_object_dtor(hlt_type_info_##tag, &obj); \
       obj = 0; \
   }

#endif
