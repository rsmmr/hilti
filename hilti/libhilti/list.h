// $Id$

#ifndef LIST_H
#define LIST_H

#include "hilti_intern.h"

typedef struct __hlt_list_node __hlt_list_node; // private
typedef struct __hlt_list_iter __hlt_list_iter; // private

struct __hlt_list_iter {
    __hlt_list* list;   
    __hlt_list_node* node; // null if at end position.
};

// Creates a new list.
extern __hlt_list* __hlt_list_new(const __hlt_type_info* elemtype, __hlt_exception* excpt);

// Inserts an element at the front. 
extern void __hlt_list_push_front(__hlt_list* l, const __hlt_type_info* type, void* val, __hlt_exception* excpt);

// Inserts an element at the end. 
extern void __hlt_list_push_back(__hlt_list* l, const __hlt_type_info* type, void* val, __hlt_exception* excpt);

// Removes and returns an element from the front of the list. 
extern void* __hlt_list_pop_front(__hlt_list* l, __hlt_exception* excpt);

// Removes and returns an element from the end of the list. 
extern void* __hlt_list_pop_back(__hlt_list* l, __hlt_exception* excpt);

// Returns the first element of the list. 
extern void* __hlt_list_front(__hlt_list* l, __hlt_exception* excpt);

// Returns the last element of the list. 
extern void* __hlt_list_back(__hlt_list* l, __hlt_exception* excpt);

// Returns the number of elements in the list.
extern int64_t __hlt_list_size(__hlt_list* l, __hlt_exception* excpt);

// Removes the element located by the iterator. 
extern void __hlt_list_erase(__hlt_list_iter i, __hlt_exception* excpt);

// Inserts a new element before the element located by the iterator.
extern void __hlt_list_insert(const __hlt_type_info* type, void* elem, __hlt_list_iter i, __hlt_exception* excpt);

// Returns an iterator positioned at the first element. 
extern __hlt_list_iter __hlt_list_begin(__hlt_list* v, __hlt_exception* excpt);

// Returns an iterator positioned right after the last element. 
extern __hlt_list_iter __hlt_list_end(__hlt_list* v, __hlt_exception* excpt);

// Advances an iterator by one position. 
extern __hlt_list_iter __hlt_list_iter_incr(const __hlt_list_iter i, __hlt_exception* excpt);

// Returns the element located by an iterator. 
extern void* __hlt_list_iter_deref(const __hlt_list_iter i, __hlt_exception* excpt);

// Returns true if two iterator locate the same element. 
extern int8_t __hlt_list_iter_eq(const __hlt_list_iter i1, const __hlt_list_iter i2, __hlt_exception* excpt);

// Converts a list into a string.
extern __hlt_string __hlt_list_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* excpt);

#endif
