///
/// Memory management.
///

#ifndef LIBHILTI_MEMORY_H
#define LIBHILTI_MEMORY_H

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
    int64_t ref_cnt;  /// The number of references to the object currently retained.
} __hlt_gchdr;

/// Statistics about the current state of memory allocations. Some are only
/// available in debugging mode.
typedef struct {
    uint64_t size_heap;     /// Current size of heap in bytes.
    uint64_t size_alloced;  /// Total number of bytes currently handed out by allocations.
    uint64_t size_stacks;   /// Total number of bytes currently allocated for stacks via hlt_alloc_stack() (debug-only).
    uint64_t num_stacks;    /// Total number of stacks currently allocated via hlt_alloc_stack() (debug-only)
    uint64_t num_allocs;    /// Total number of calls to allocation functions (debug-only).
    uint64_t num_deallocs;  /// Total number of calls to deallocation functions (debug-only).
    uint64_t num_refs;      /// Total number of reference count increments (debug-only).
    uint64_t num_unrefs;    /// Total number of reference count decrements (debug-only).
} hlt_memory_stats;

/// Returns statistics about the current state of memory allocations.
hlt_memory_stats hlt_memory_statistics();

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

/// XXX
extern void* __hlt_malloc_no_init(uint64_t size, const char* type, const char* location);

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
/// memory must be freed with ``hlt_free``. The added memory will be null
/// initialized.
///
/// ptr: The pointer to reallocate. This must have been allocated with
/// __hlt_malloc().
///
/// size: The size of bytes to reallocate.
///
/// old_size: The old size, i.e., the number of bytes currently allocated at
/// \a ptr.
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
extern void* __hlt_realloc(void* ptr, uint64_t size, uint64_t old_size, const char* type, const char* location);

/// XXXX
extern void* __hlt_realloc_no_init(void* ptr, uint64_t size, const char* type, const char* location);

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
extern void __hlt_free(void* ptr, const char* type, const char* location);

/// Allocates a chunk of stack space. This is handled separated from other
/// memory allocation to allow to internally optimze for large allocations of
/// memory that may be used only partially. It internally uses mmap(). The
/// function won't return if the allocation fails, but abort execution.
///
/// size: The size of the stack space to allocate.
///
/// Returns: The allocated space. The memory will be zero-initialized.
extern void* hlt_stack_alloc(size_t size);

/// Frees a stack allocated via hlt_alloc_stack().
///
/// stack: The memory allocated by hlt_stack_alloc().
///
/// size: The size that was specified for the corresponding hlt_alloc_stack()
/// call.
extern void hlt_stack_free(void* stack, size_t size);

/// Invalidates a stack's memory without releasing it. This may allow the OS
/// to recycle previously allocated physical memory.
///
/// stack: The memory allocated by hlt_stack_alloc().
///
/// size: The size that was specified for the corresponding hlt_alloc_stack()
/// call.
extern void hlt_stack_invalidate(void* stack, size_t size);

#define hlt_malloc(size)                 __hlt_malloc(size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_malloc_no_init(size)         __hlt_malloc_no_init(size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_calloc(count, size)          __hlt_calloc(count, size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_realloc(ptr, size, old_size) __hlt_realloc(ptr, size, old_size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_realloc_no_init(ptr, size)   __hlt_realloc_no_init(ptr, size, "-", __hlt_make_location(__FILE__,__LINE__))
#define hlt_free(ptr)                    __hlt_free(ptr, "-", __hlt_make_location(__FILE__,__LINE__))

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
extern void* __hlt_object_new_no_init(const hlt_type_info* ti, uint64_t size, const char* location);

/// XXX
#define GC_ASSIGN(obj, val, tag) \
   { \
   tag* tmp = val; \
   __hlt_object_cctor(&hlt_type_info_##tag, (void*)&val, __hlt_make_location(__FILE__,__LINE__)); \
   __hlt_object_dtor(&hlt_type_info_##tag, (void*)&obj, __hlt_make_location(__FILE__,__LINE__)); \
   obj = tmp; \
   }

/// XXX
#define GC_ASSIGN_REFED(obj, val, tag) \
   { \
   tag* tmp = val; \
   __hlt_object_dtor(&hlt_type_info_##tag, (void*)&obj, __hlt_make_location(__FILE__,__LINE__)); \
   obj = tmp; \
   }

/// XXX
#define GC_INIT(obj, val, tag) \
   { \
   obj = val; \
   __hlt_object_cctor(&hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_NEW(tag) \
   __hlt_object_new(&hlt_type_info_##tag, sizeof(tag), __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_NEW_NO_INIT(tag) \
   __hlt_object_new_no_init(&hlt_type_info_##tag, sizeof(tag), __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_NEW_CUSTOM_SIZE(tag, size) \
   __hlt_object_new(&hlt_type_info_##tag, size, __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_NEW_CUSTOM_SIZE_NO_INIT(tag, size) \
   __hlt_object_new_no_init(&hlt_type_info_##tag, size, __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_NEW_CUSTOM_SIZE_GENERIC(ti, size) \
   __hlt_object_new(ti, size, __hlt_make_location(__FILE__,__LINE__));

/// XXX
#define GC_DTOR(obj, tag) \
   { \
       __hlt_object_dtor(&hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_DTOR_GENERIC(objptr, ti) \
   { \
       __hlt_object_dtor(ti, objptr, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_CCTOR(obj, tag) \
   { \
       __hlt_object_cctor(&hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_CCTOR_GENERIC(objptr, ti) \
   { \
       __hlt_object_cctor(ti, objptr, __hlt_make_location(__FILE__,__LINE__)); \
   }

/// XXX
#define GC_CLEAR(obj, tag) \
   { \
       __hlt_object_dtor(&hlt_type_info_##tag, &obj, __hlt_make_location(__FILE__,__LINE__)); \
       obj = 0; \
   }

typedef struct __hlt_free_list_block {
    struct __hlt_free_list_block* next;
    char data[];
} __hlt_free_list_block;

typedef struct {
    __hlt_free_list_block* pool;
    size_t size; // For ensuring the size arguments remain consistent.
} hlt_free_list;

/// Creates a new free list managing memory objects.
///
/// size: The size of the blocks to allocate.
///
/// Returns: A pointer to the new free list object.
hlt_free_list* hlt_free_list_new(size_t size);

/// Returns the block size of a free list.
///
/// list: The list to return the block size for.
size_t hlt_free_list_block_size(hlt_free_list* list);

/// Allocates a new chunk of memory. If the free list has currently unused
/// blocks, one of them will be returned; otherwise, a new one will be
/// allocated on the heap.
///
/// list: The list to allocate from.
///
/// Returns: A pointer of size at least \a size.
///
/// \note This will abort execution if it can't satisfy the request.
void* hlt_free_list_alloc(hlt_free_list* list);

/// Frees a chunk of memory formerly returned by hlt_free_list_alloc(). It
/// will be returned to the pool.
///
/// list: The list to return the block to.
///
/// p: The memory to return to the pool.
void hlt_free_list_free(hlt_free_list* list, void* p);

/// Deletes a free list, freeing all resources still pooled (but not those
/// currently handed out) as well as the the free list itself.
///
/// list: The list to destroy.
void hlt_free_list_deletes(hlt_free_list* list);

/// XXX

#define HLT_MEMORY_POOL_INLINE(name, size) \
   hlt_memory_pool mpool;           \
   char __ ## name ## _data[size];

#define HLT_MEMORY_POOL_INLINE_INIT(obj, name) \
   hlt_memory_pool_init(&obj->name, sizeof(obj->__ ## name ## _data))

typedef struct __hlt_memory_pool_block {
    struct __hlt_memory_pool_block* next;
    char* cur;
    char* end;
    char data[0];
} __hlt_memory_pool_block;

typedef struct {
    __hlt_memory_pool_block first; // We store the first inline.
    __hlt_memory_pool_block* last;
} hlt_memory_pool;


// XXX Allocations are fast. All allocations part of a pool will be released
// on dtor.
void  hlt_memory_pool_init(hlt_memory_pool* p, size_t size);
void  hlt_memory_pool_dtor(hlt_memory_pool* p);
void* hlt_memory_pool_malloc(hlt_memory_pool* p, size_t n);
void* hlt_memory_pool_calloc(hlt_memory_pool* p, size_t count, size_t n);
void  hlt_memory_pool_free(hlt_memory_pool* p, void* b); // just a hint, not mandatory

#endif
