// $Id$
//
// FIXME: Vector's don't shrink at the moment. When should they?

#include <string.h>

#include "hilti.h"

// Factor by which to growth array on reallocation. 
static const float GrowthFactor = 1.5;

// Initial allocation size for a new vector
static const hlt_vector_idx InitialCapacity = 1;

struct hlt_vector {
    void *elems;                 // Pointer to the element array.
    hlt_vector_idx last;       // Largest valid index
    hlt_vector_idx capacity;   // Number of element we have physically allocated in elems.
    const hlt_type_info* type; // Type information for our elements
    void* def;                   // Default element for not yet initialized fields. 
};

hlt_vector* hlt_vector_new(const hlt_type_info* elemtype, const void* def, hlt_exception* excpt)
{
    hlt_vector* v = hlt_gc_malloc_non_atomic(sizeof(hlt_vector));
    if ( ! v ) {
        *excpt = hlt_exception_out_of_memory;
        return 0;
    }
    
    v->elems = hlt_gc_malloc_non_atomic(elemtype->size * InitialCapacity);
    if ( ! v->elems ) {
        *excpt = hlt_exception_out_of_memory;
        return 0;
    }

    // We need to deep-copy the default element as the caller might have it
    // on its stack. 
    v->def = hlt_gc_malloc_non_atomic(elemtype->size);
    if ( ! v->def ) {
        *excpt = hlt_exception_out_of_memory;
        return 0;
    }
    
    memcpy(v->def, def, elemtype->size);
    
    v->last = -1;
    v->capacity = InitialCapacity;
    v->type = elemtype;
    
    return v;
}

void* hlt_vector_get(hlt_vector* v, hlt_vector_idx i, hlt_exception* excpt)
{
    if ( i > v->last ) {
        *excpt = hlt_exception_index_error;
        return 0;
    }
    
    return v->elems + i * v->type->size;
}

#include <stdio.h>

void hlt_vector_set(hlt_vector* v, hlt_vector_idx i, const hlt_type_info* elemtype, void* val, hlt_exception* excpt)
{
    assert(elemtype == v->type);
    
    if ( i >= v->capacity ) {
        // Allocate more memory. 
        hlt_vector_idx c = v->capacity;
        while ( i >= c )
            c *= (c+1) * GrowthFactor; 
        
        hlt_vector_reserve(v, c, excpt);
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

hlt_vector_idx hlt_vector_size(hlt_vector* v, hlt_exception* excpt)
{
    return v->last + 1;
}

void hlt_vector_reserve(hlt_vector* v, hlt_vector_idx n, hlt_exception* excpt)
{
    if ( v->capacity >= n )
        return;
    
    v->elems = hlt_gc_realloc_non_atomic(v->elems, v->type->size * n);
    if ( ! v->elems ) {
        *excpt = hlt_exception_out_of_memory;
        return;
    }
    
    v->capacity = n;
}

hlt_vector_iter hlt_vector_begin(const hlt_vector* v, hlt_exception* excpt)
{
    hlt_vector_iter i;
    i.vec = v->last >= 0 ? v : 0;
    i.idx = 0;
    return i;
}

hlt_vector_iter hlt_vector_end(const hlt_vector* v, hlt_exception* excpt)
{
    hlt_vector_iter i;
    i.vec = 0;
    i.idx = 0;
    return i;
}

hlt_vector_iter hlt_vector_iter_incr(const hlt_vector_iter i, hlt_exception* excpt)
{
    if ( ! i.vec )
        return i;
    
    hlt_vector_iter j = i;
    ++j.idx;
    if ( j.idx > j.vec->last ) {
        // End reached.
        j.vec = 0;
        j.idx = 0;
    }
         
    return j;
}

void* hlt_vector_iter_deref(const hlt_vector_iter i, hlt_exception* excpt)
{
    if ( ! i.vec ) {
        *excpt = hlt_exception_invalid_iterator;
        return 0;
    }
         
    return i.vec->elems + i.idx * i.vec->type->size;
}

int8_t hlt_vector_iter_eq(const hlt_vector_iter i1, const hlt_vector_iter i2, hlt_exception* excpt)
{
    return i1.vec == i2.vec && i1.idx == i2.idx;
}


