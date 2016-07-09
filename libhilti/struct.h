///
/// Support functions for HILTI's struct data type.
///

#ifndef LIBHILTI_STRUCT_H
#define LIBHILTI_STRUCT_H

#include "types.h"

/// Meta-information about a struct field.
typedef struct {
    const hlt_type_info* type; ///< The type of the field.
    const char* name;          ///< The name of the field.
} hlt_struct_field;

/// Converts a HILTI struct into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_struct_to_string(const hlt_type_info* type, void* obj, int32_t options,
                                       __hlt_pointer_stack* seen, hlt_exception** excpt,
                                       hlt_execution_context* ctx);

/// Returns the number of fields a struct type has.
///
/// type: The struct type.
extern int hlt_struct_size(const hlt_type_info* type, hlt_exception** excpt,
                           hlt_execution_context* ctx);

/// Checks if a struct field is set.
///
/// type: The type of the struct.
///
/// obj: A pointer to the struct.
///
/// index: The zero-based index of the element to return.
///
/// \hlt_c
///
/// Returns: True if the field at the given index is set.
extern int8_t hlt_struct_is_set(const hlt_type_info* type, void* obj, int index,
                                hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns a pointer to a field at a given index.
///
/// type: The type of the struct.
///
/// obj: A pointer to the struct.
///
/// index: The zero-based index of the element to return.
///
/// \hlt_c
///
/// Returns: A pointer to the field at the given index.  The content of data
/// being pointed to is undefined if the field isn't set.
extern void* hlt_struct_get(const hlt_type_info* type, void* obj, int index, hlt_exception** excpt,
                            hlt_execution_context* ctx);

/// Returns the type and other meta information a the struct field at a given
/// index.
///
/// type: The type of the struct.
///
/// index: The zero-based index of the element to return.
///
/// \hlt_c
///
/// Returns: A pointer to the meta information. All pointers in there must be left untouched,
/// ownership is *not* passed to caller.
extern hlt_struct_field hlt_struct_get_type(const hlt_type_info* type, int index,
                                            hlt_exception** excpt, hlt_execution_context* ctx);

#endif
