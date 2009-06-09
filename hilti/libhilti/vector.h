// $Id$

#ifndef VECTOR_H
#define VECTOR_H

#include "hilti_intern.h"

typedef int64_t __hlt_vector_idx;
typedef struct __hlt_vector __hlt_vector;
typedef struct __hlt_vector_iter __hlt_vector_iter; // private

struct __hlt_vector_iter {
    const __hlt_vector* vec; 
    __hlt_vector_idx idx;
};

// Creates a new vector. *def* is an element which used as the default for
// all not-initialized elements.
extern __hlt_vector* __hlt_vector_new(const __hlt_type_info* elemtype, const void* def, __hlt_exception* excpt);

// Returns the element at the given index.
extern void* __hlt_vector_get(__hlt_vector* v, __hlt_vector_idx i, __hlt_exception* excpt);

// Sets the element at the given index to the value.
extern void __hlt_vector_set(__hlt_vector* v, __hlt_vector_idx i, const __hlt_type_info* elemtype, void* val, __hlt_exception* excpt);

// Returns the size of the vector (i.e., the largest valid index + 1 )
extern __hlt_vector_idx __hlt_vector_size(__hlt_vector* v, __hlt_exception* excpt);

// Reserves space for at least n elements in the vector. Note that you still
// can't access any element beyond the largest initialized index. This is
// mainly a hint to avoid unnecessary reallocation.
extern void __hlt_vector_reserve(__hlt_vector* v, __hlt_vector_idx n, __hlt_exception* excpt);

// Returns an iterator positioned at the first element. 
extern __hlt_vector_iter __hlt_vector_begin(const __hlt_vector* v, __hlt_exception* excpt);

// Returns an iterator positioned right after the last element. 
extern __hlt_vector_iter __hlt_vector_end(const __hlt_vector* v, __hlt_exception* excpt);

// Advances an iterator by one position. 
extern __hlt_vector_iter __hlt_vector_iter_incr(const __hlt_vector_iter i, __hlt_exception* excpt);

// Returns the element located by an iterator. 
extern void* __hlt_vector_iter_deref(const __hlt_vector_iter i, __hlt_exception* excpt);

// Returns true if two iterator locate the same element. 
extern int8_t __hlt_vector_iter_eq(const __hlt_vector_iter i1, const __hlt_vector_iter i2, __hlt_exception* excpt);

#endif
