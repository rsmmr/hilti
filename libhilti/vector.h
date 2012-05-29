
#ifndef HILTI_VECTOR_H
#define HILTI_VECTOR_H

#include "exceptions.h"
#include "interval.h"
#include "enum.h"

typedef int64_t hlt_vector_idx;
typedef struct __hlt_vector hlt_vector;
typedef struct __hlt_iterator_vector hlt_iterator_vector;

struct __hlt_iterator_vector {
    hlt_vector* vec;
    hlt_vector_idx idx;
};

struct __hlt_timer_mgr;

/// Cookie for entry expiration timers.
typedef struct __hlt_iterator_vector __hlt_vector_timer_cookie;

// Creates a new vector. *def* is an element which used as the default for
// all not-initialized elements.
extern hlt_vector* hlt_vector_new(const hlt_type_info* elemtype, const void* def, struct __hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx);

/// Actives automatic expiration of vector entries.
extern void hlt_vector_timeout(hlt_vector* v, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the element at the given index.
extern void* hlt_vector_get(hlt_vector* v, hlt_vector_idx i, hlt_exception** excpt, hlt_execution_context* ctx);

// Sets the element at the given index to the value.
extern void hlt_vector_set(hlt_vector* v, hlt_vector_idx i, const hlt_type_info* elemtype, void* val, hlt_exception** excpt, hlt_execution_context* ctx);

// Appends the element to the vector.
extern void hlt_vector_push_back(hlt_vector* v, const hlt_type_info* elemtype, void* val, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the size of the vector (i.e., the largest valid index + 1 )
extern hlt_vector_idx hlt_vector_size(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Reserves space for at least n elements in the vector. Note that you still
// can't access any element beyond the largest initialized index. This is
// mainly a hint to avoid unnecessary reallocation.
extern void hlt_vector_reserve(hlt_vector* v, hlt_vector_idx n, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns an iterator positioned at the first element.
extern hlt_iterator_vector hlt_vector_begin(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns an iterator positioned right after the last element.
extern hlt_iterator_vector hlt_vector_end(hlt_vector* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Advances an iterator by one position.
extern hlt_iterator_vector hlt_iterator_vector_incr(hlt_iterator_vector i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the element located by an iterator.
extern void* hlt_iterator_vector_deref(hlt_iterator_vector i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns true if two iterator locate the same element.
extern int8_t hlt_iterator_vector_eq(hlt_iterator_vector i1, hlt_iterator_vector i2, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a vector into a string.
extern hlt_string hlt_vector_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx);

/// Called by an expiring timer to remove an element from the vector.
///
/// cookie: The cookie identifying the element to be removed.
extern void hlt_vector_expire(__hlt_vector_timer_cookie cookie);

#endif
