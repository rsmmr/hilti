// $Id$

#ifndef HILTI_VECTOR_H
#define HILTI_VECTOR_H

#include "exceptions.h"

typedef int64_t hlt_vector_idx;
typedef struct hlt_vector hlt_vector;
typedef struct hlt_vector_iter hlt_vector_iter; // private

struct hlt_vector_iter {
    const hlt_vector* vec;
    hlt_vector_idx idx;
};

// Creates a new vector. *def* is an element which used as the default for
// all not-initialized elements.
extern hlt_vector* hlt_vector_new(const hlt_type_info* elemtype, const void* def, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the element at the given index.
extern void* hlt_vector_get(hlt_vector* v, hlt_vector_idx i, hlt_exception** excpt, hlt_execution_context* ctx);

// Sets the element at the given index to the value.
extern void hlt_vector_set(hlt_vector* v, hlt_vector_idx i, const hlt_type_info* elemtype, void* val, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the size of the vector (i.e., the largest valid index + 1 )
extern hlt_vector_idx hlt_vector_size(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Reserves space for at least n elements in the vector. Note that you still
// can't access any element beyond the largest initialized index. This is
// mainly a hint to avoid unnecessary reallocation.
extern void hlt_vector_reserve(hlt_vector* v, hlt_vector_idx n, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns an iterator positioned at the first element.
extern hlt_vector_iter hlt_vector_begin(const hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns an iterator positioned right after the last element.
extern hlt_vector_iter hlt_vector_end(const hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Advances an iterator by one position.
extern hlt_vector_iter hlt_vector_iter_incr(const hlt_vector_iter i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the element located by an iterator.
extern void* hlt_vector_iter_deref(const hlt_vector_iter i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns true if two iterator locate the same element.
extern int8_t hlt_vector_iter_eq(const hlt_vector_iter i1, const hlt_vector_iter i2, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
