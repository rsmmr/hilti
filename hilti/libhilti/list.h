// $Id$

#ifndef HILTI_LIST_H
#define HILTI_LIST_H

#include "exceptions.h"

typedef struct __hlt_list hlt_list;
typedef struct __hlt_list_node __hlt_list_node;
typedef struct __hlt_list_iter hlt_list_iter;

struct __hlt_list_iter {
    hlt_list* list;   
    __hlt_list_node* node; // null if at end position.
};

// Creates a new list.
extern hlt_list* hlt_list_new(const hlt_type_info* elemtype, hlt_exception* excpt);

// Inserts an element at the front. 
extern void hlt_list_push_front(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception* excpt);

// Inserts an element at the end. 
extern void hlt_list_push_back(hlt_list* l, const hlt_type_info* type, void* val, hlt_exception* excpt);

// Removes and returns an element from the front of the list. 
extern void* hlt_list_pop_front(hlt_list* l, hlt_exception* excpt);

// Removes and returns an element from the end of the list. 
extern void* hlt_list_pop_back(hlt_list* l, hlt_exception* excpt);

// Returns the first element of the list. 
extern void* hlt_list_front(hlt_list* l, hlt_exception* excpt);

// Returns the last element of the list. 
extern void* hlt_list_back(hlt_list* l, hlt_exception* excpt);

// Returns the number of elements in the list.
extern int64_t hlt_list_size(hlt_list* l, hlt_exception* excpt);

// Removes the element located by the iterator. 
extern void hlt_list_erase(hlt_list_iter i, hlt_exception* excpt);

// Inserts a new element before the element located by the iterator.
extern void hlt_list_insert(const hlt_type_info* type, void* elem, hlt_list_iter i, hlt_exception* excpt);

// Returns an iterator positioned at the first element. 
extern hlt_list_iter hlt_list_begin(hlt_list* v, hlt_exception* excpt);

// Returns an iterator positioned right after the last element. 
extern hlt_list_iter hlt_list_end(hlt_list* v, hlt_exception* excpt);

// Advances an iterator by one position. 
extern hlt_list_iter hlt_list_iter_incr(const hlt_list_iter i, hlt_exception* excpt);

// Returns the element located by an iterator. 
extern void* hlt_list_iter_deref(const hlt_list_iter i, hlt_exception* excpt);

// Returns true if two iterator locate the same element. 
extern int8_t hlt_list_iter_eq(const hlt_list_iter i1, const hlt_list_iter i2, hlt_exception* excpt);

// Converts a list into a string.
extern hlt_string hlt_list_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception* excpt);

#endif
