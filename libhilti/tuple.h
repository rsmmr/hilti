///
/// Support functions for HILTI's tuple data type.
///

#ifndef LIBHILTI_TUPLE_H
#define LIBHILTI_TUPLE_H

#include "types.h"

/// Meta-information about a tuple element.
typedef struct {
    const hlt_type_info* type; ///< The type of the element.
    const char* name;          ///< The name of the element, empty string if not set.
} hlt_tuple_element;

/// Returns the number of elements a tuple type has.
///
/// type: The type of the tuple.
///
/// \hlt_c
extern int hlt_tuple_length(const hlt_type_info* type, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a pointer to a tuple element at a given index.
///
/// type: The type of the tuple.
///
/// obj: A pointer to the tuple.
///
/// idex: The zero-based index of the element to return.
///
/// \hlt_c
extern void* hlt_tuple_get(const hlt_type_info* type, void* obj, int index, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the type of the tuple element at a given index.
///
/// type: The type of the tuple.
///
/// index: The zero-based index of the element to return.
///
/// \hlt_c
extern hlt_tuple_element hlt_tuple_get_type(const hlt_type_info* type, int index, hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a HILTI tuple into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_tuple_to_string(const hlt_type_info* type, void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

#endif


