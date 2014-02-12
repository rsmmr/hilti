// $Id$

#ifndef LIBHILTI_LIST_H
#define LIBHILTI_LIST_H

#include "exceptions.h"

typedef struct __hlt_list_node __hlt_list_node;
typedef struct __hlt_iterator_list hlt_iterator_list;

struct __hlt_iterator_list {
    hlt_list* list;
    __hlt_list_node* node; // null if at end position.
};

struct __hlt_timer_mgr;

/// Cookie for entry expiration timers.
typedef struct __hlt_iterator_list __hlt_list_timer_cookie;

// Creates a new list.
extern hlt_list* hlt_list_new(const hlt_type_info* elemtype, struct __hlt_timer_mgr* tmgr, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the type of list elements.
extern const hlt_type_info* hlt_list_type(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx);

/// Actives automatic expiration of list entries.
// extern void hlt_list_timeout(hlt_list* l, hlt_enum strategy, hlt_interval timeout, hlt_exception** excpt, hlt_execution_context* ctx);

// Inserts an element at the front.
extern void hlt_list_push_front(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception** excpt, hlt_execution_context* ctx);

// Inserts an element at the end.
extern void hlt_list_push_back(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception** excpt, hlt_execution_context* ctx);

// Removes an element from the front of the list.
extern void hlt_list_pop_front(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx);

// Removes an element from the end of the list.
extern void hlt_list_pop_back(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the first element of the list.
extern void* hlt_list_front(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the last element of the list.
extern void* hlt_list_back(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx);

// Appends all elements of l2 to l1.
extern void hlt_list_append(hlt_list* l1, hlt_list* l2, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the number of elements in the list.
extern int64_t hlt_list_size(hlt_list* l, hlt_exception** excpt, hlt_execution_context* ctx);

// Removes the element located by the iterator.
extern void hlt_list_erase(hlt_iterator_list i, hlt_exception** excpt, hlt_execution_context* ctx);

// Expires the entry the iterator references from the list.
extern void hlt_iterator_list_expire(hlt_iterator_list i, hlt_exception** excpt, hlt_execution_context* ctx);

// Inserts a new element before the element located by the iterator.
extern void hlt_list_insert(const hlt_type_info* type, void* elem, hlt_iterator_list i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns an iterator positioned at the first element.
extern hlt_iterator_list hlt_list_begin(hlt_list* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns an iterator positioned right after the last element.
extern hlt_iterator_list hlt_list_end(hlt_list* v, hlt_exception** excpt, hlt_execution_context* ctx);

// Advances an iterator by one position.
extern hlt_iterator_list hlt_iterator_list_incr(const hlt_iterator_list i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns the element located by an iterator.
extern void* hlt_iterator_list_deref(const hlt_iterator_list i, hlt_exception** excpt, hlt_execution_context* ctx);

// Returns true if two iterator locate the same element.
extern int8_t hlt_iterator_list_eq(const hlt_iterator_list i1, const hlt_iterator_list i2, hlt_exception** excpt, hlt_execution_context* ctx);

// Converts a list into a string.
extern hlt_string hlt_list_to_string(const hlt_type_info* type, const void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

/// Called by an expiring timer to remove an element from the list.
///
/// cookie: The cookie identifying the element to be removed.
extern void hlt_list_expire(__hlt_list_timer_cookie cookie);

#endif
