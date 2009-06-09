// $Id$
//
// FIXME: Vector's don't shrink at the moment. When should they?

#include <string.h>

#include "hilti_intern.h"

// Factor by which to growth array on reallocation. 
static const float GrowthFactor = 1.5;

// Initial allocation size for a new vector
static const __hlt_vector_idx InitialCapacity = 1;

struct __hlt_vector {
    void *elems;                 // Pointer to the element array.
    __hlt_vector_idx last;       // Largest valid index
    __hlt_vector_idx capacity;   // Number of element we have physically allocated in elems.
    const __hlt_type_info* type; // Type information for our elements
    void* def;                   // Default element for not yet initialized fields. 
};

__hlt_vector* __hlt_vector_new(const __hlt_type_info* elemtype, const void* def, __hlt_exception* excpt)
{
    __hlt_vector* v = __hlt_gc_malloc_non_atomic(sizeof(__hlt_vector));
    if ( ! v ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }
    
    v->elems = __hlt_gc_malloc_non_atomic(elemtype->size * InitialCapacity);
    if ( ! v->elems ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }

    // We need to deep-copy the default element as the caller might have it
    // on its stack. 
    v->def = __hlt_gc_malloc_non_atomic(elemtype->size);
    if ( ! v->def ) {
        *excpt = __hlt_exception_out_of_memory;
        return 0;
    }
    
    memcpy(v->def, def, elemtype->size);
    
    v->last = -1;
    v->capacity = InitialCapacity;
    v->type = elemtype;
    
    return v;
}

void* __hlt_vector_get(__hlt_vector* v, __hlt_vector_idx i, __hlt_exception* excpt)
{
    if ( i > v->last ) {
        *excpt = __hlt_exception_index_error;
        return 0;
    }
    
    return v->elems + i * v->type->size;
}

#include <stdio.h>

void __hlt_vector_set(__hlt_vector* v, __hlt_vector_idx i, const __hlt_type_info* elemtype, void* val, __hlt_exception* excpt)
{
    assert(elemtype == v->type);
    
    if ( i >= v->capacity ) {
        // Allocate more memory. 
        __hlt_vector_idx c = v->capacity;
        while ( i >= c )
            c *= (c+1) * GrowthFactor; 
        
        __hlt_vector_reserve(v, c, excpt);
        if ( *excpt )
            return;
    }
    
    for ( int j = v->last + 1; j < i; j++ ) {
        // Initialize element between old and new end of vector.
        void* dst = v->elems + j * v->type->size;
        memcpy(dst, v->def, v->type->size);
    }
    
    if ( i > v->last )
        v->last = i;
    
    // Copy new value into vector.
    void* dst = v->elems + i * v->type->size;
    memcpy(dst, val, v->type->size);
}

__hlt_vector_idx __hlt_vector_size(__hlt_vector* v, __hlt_exception* excpt)
{
    return v->last + 1;
}

void __hlt_vector_reserve(__hlt_vector* v, __hlt_vector_idx n, __hlt_exception* excpt)
{
    if ( v->capacity >= n )
        return;
    
    v->elems = __hlt_gc_realloc_non_atomic(v->elems, v->type->size * n);
    if ( ! v->elems ) {
        *excpt = __hlt_exception_out_of_memory;
        return;
    }
    
    v->capacity = n;
}

__hlt_vector_iter __hlt_vector_begin(const __hlt_vector* v, __hlt_exception* excpt)
{
    __hlt_vector_iter i;
    i.vec = v->last >= 0 ? v : 0;
    i.idx = 0;
    return i;
}

__hlt_vector_iter __hlt_vector_end(const __hlt_vector* v, __hlt_exception* excpt)
{
    __hlt_vector_iter i;
    i.vec = 0;
    i.idx = 0;
    return i;
}

__hlt_vector_iter __hlt_vector_iter_incr(const __hlt_vector_iter i, __hlt_exception* excpt)
{
    if ( ! i.vec )
        return i;
    
    __hlt_vector_iter j = i;
    ++j.idx;
    if ( j.idx > j.vec->last ) {
        // End reached.
        j.vec = 0;
        j.idx = 0;
    }
         
    return j;
}

void* __hlt_vector_iter_deref(const __hlt_vector_iter i, __hlt_exception* excpt)
{
    if ( ! i.vec ) {
        *excpt = __hlt_exception_invalid_iterator;
        return 0;
    }
         
    return i.vec->elems + i.idx * i.vec->type->size;
}

int8_t __hlt_vector_iter_eq(const __hlt_vector_iter i1, const __hlt_vector_iter i2, __hlt_exception* excpt)
{
    return i1.vec == i2.vec && i1.idx == i2.idx;
}


