///
/// Memory management.
///

#ifndef HILTI_MEMORY_H
#define HILTI_MEMORY_H

#include "types.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef DEBUG
extern const char* __hlt_make_location(const char* file, int line);
#else
#define __hlt_make_location(file, line) 0
#endif

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
/// type: A string describing the type of object being allocated. This is for
/// debug purposes only.
///
/// location: A string describing the location where the object is allocaged.
/// This is for debugging purposes only.
///
/// Returns: A pointer to the new memory chunk. This will never be null.
///
/// \note This shouldn't be called directly by user code. Use the macro \c
/// hlt_malloc instead.
extern void* __hlt_malloc(uint64_t size, const char* type, const char* location);

/// Allocates a number of unmanaged memory elements of a given size. This
/// operates pretty much like calloc but it will always return a valid
/// address. If it can't allocate sufficient memory, the function will
/// terminate the current process directly and not return. The allocated
/// memory must be freed with ``hlt_free``.
///
/// count: The number of elements to allocate.
///
/// size: The size of one element being allocated.
///
/// type: A string describing the type of object being allocated. This is for
/// debug purposes only.
///
/// location: A string describing the location where the object is allocaged.
/// This is for debugging purposes only.
///
/// Returns: A pointer to the new memory chunk. This will never be null.
///
/// \note This shouldn't be called directly by user code. Use the macro \c
/// hlt_calloc instead.
extern void* __hlt_calloc(uint64_t count, uint64_t size, const char* type, const char* location);

/// Reallocates a number of unmanaged memory elements of a given size. This
/// operates pretty much like realloc but it will always return a valid
/// address. If it can't allocate sufficient memory, the function will
/// terminate the current process directly and not return. The reallocated
/// memory must be freed with ``hlt_free``.
///
/// ptr: The pointer to reallocate. This must have been allocated with
/// __hlt_malloc().
///
/// size: The size of bytes to reallocate.
///
/// type: A string describing the type of object being allocated. This is for
/// debug purposes only.
///
/// location: A string describing the location where the object is allocaged.
/// This is for debugging purposes only.
///
/// Returns: A pointer to the new memory chunk. This will never be null.
///
/// \note This shouldn't be called directly by user code. Use the macro \c
/// hlt_calloc instead.
extern void* __hlt_realloc(void *ptr, uint64_t size, const char* type, const char* location);

/// Frees an unmanaged memory chunk formerly allocated with ``hlt_malloc``.
/// This operates pretty much like the standard ``free``.
///
/// ptr: The memory chunk to free. Can be null, in which case the function
/// will not do anything.
///
/// type: A string describing the type of object being allocated. This is for
/// debug purposes only. This string should match what is passed to the
/// corresponding \c __hlt_malloc/hlt_calloc call.
///
/// location: A string describing the location where the object is allocaged.
/// This is for debugging purposes only.
extern void __hlt_free(void *ptr, const char* type, const char* location);

#define hlt_malloc(size)        __hlt_malloc(size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_calloc(count, size) __hlt_calloc(count, size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_realloc(ptr, size)  __hlt_realloc(ptr, size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_free(ptr)           __hlt_free(ptr, "-", __hlt_make_location(__FILE__,__LINE__))

// Internal function to increaes a memory managed object's reference count by
// one. Not to be used directly from user code.
extern void __hlt_object_ref(const hlt_type_info* ti, void* obj);

// Internal function to decrease a memory managed object's reference count by
// one. Not to be used directly from user code.
extern void __hlt_object_unref(const hlt_type_info* ti, void* obj);

/// XXX
extern void __hlt_object_dtor(const hlt_type_info* ti, void* obj, const char* location);

/// XXX
extern void __hlt_object_cctor(const hlt_type_info* ti, void* obj, const char* location);

/// XXX
extern void* __hlt_object_new(const hlt_type_info* ti, uint64_t size, const char* location);

/// XXX
#define GC_ASSIGN(obj, val, tag) \
   { \
   tag* tmp = val; \
   __hlt_object_cctor(hlt_type_info_##tag, (void*)&val, __hlt_make_location(__FILE__,__LINE__)); \
   __hlt_object_dtor(hlt_type_info_##tag, (void*)&obj, __hlt_make_location(__FILE__,__LINE__)); \
   obj = tmp; \
   }

/// XXX
#define GC_ASSIGN_REFED(obj, val, tag) \
   { \
   tag* tmp = val; \
   __hlt_object_dtor(hlt_type_info_##tag, (void*)&obj, __hlt_make_location(__FILE__,__LINE__)); \
   obj = tmp; \
   }

/// XXX
#define GC_INIT(obj, val, tag) \
   { \
   obj = val; \
   __hlt_object_cctor(hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_NEW(tag) \
   __hlt_object_new(hlt_type_info_##tag, sizeof(tag), __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_NEW_CUSTOM_SIZE(tag, size) \
   __hlt_object_new(hlt_type_info_##tag, size, __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_DTOR(obj, tag) \
   { \
       __hlt_object_dtor(hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_DTOR_GENERIC(objptr, ti) \
   { \
       __hlt_object_dtor(ti, objptr, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_CCTOR(obj, tag) \
   { \
       __hlt_object_cctor(hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_CCTOR_GENERIC(objptr, ti) \
   { \
       __hlt_object_cctor(ti, objptr, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_CLEAR(obj, tag) \
   { \
       __hlt_object_dtor(hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
       obj = 0; \
   }

typedef struct __hlt_free_list_block {
    struct __hlt_free_list_block* next;
    char data[];
} __hlt_free_list_block;

typedef struct {
    __hlt_free_list_block* pool;
#ifdef DEBUG
    size_t size; // For ensuring the size arguments remain consistent.
#endif
} hlt_free_list;

/// Create a new free list managing memory objects. Note that it creates the
/// free list in place at the address passed in.
///
/// list: A pointer to a free list object to initialize.
void hlt_free_list_init(hlt_free_list* list);

/// Allocates a new chunk of memory. If the free list has currently unused
/// blocks, one of them will be returned; otherwise, a new one will be
/// allocated on the heap.
///
/// list: The list to allocate from.
///
/// size: The size of the block to allocate. Note that this must be constant
/// across all calls, or results are undefined.
///
/// Returns: A pointer of size at least \a size.
///
/// \note This will abort execution if it can't satisfy the request.
void* hlt_free_list_alloc(hlt_free_list* list, size_t size);

/// Frees a chunk of memory formerly returned by hlt_free_list_alloc(). It
/// will be returned to the pool.
///
/// list: The list to return the block to.
///
/// size: The size of the block freed; this must match what was past to
/// hlt_free_list_alloc(), or result will be undefined.
///
/// p: The memory to return to the pool.
void hlt_free_list_free(hlt_free_list* list, void* p, size_t size);

/// Destroys a free list, freeing all resources still pooled (but not those
/// currently handed out). Does not free the free-list objects itself.
///
/// list: The list to destroy.
void hlt_free_list_destroy(hlt_free_list* list);

#endif
