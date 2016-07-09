///
/// Support functions for HILTI's union data type.
///

#ifndef LIBHILTI_UNION_H
#define LIBHILTI_UNION_H

#include "types.h"

struct __hlt_union;
typedef struct __hlt_union hlt_union;

/// Meta-information about a union field.
typedef struct {
    const hlt_type_info* type; ///< The type of the field.
    const char* name;          ///< The name of the field.
} hlt_union_field;

/// Checks if a unions' value is set to a specified field.
///
/// u: The union.
///
/// idx: The field to test. If -1, checks if any field is set.
///
/// \hlt_c
///
/// Returns: True if the union is set.
extern int8_t hlt_union_is_set(hlt_union* u, int64_t idx, hlt_exception** excpt,
                               hlt_execution_context* ctx);

/// Returns a pointer to the value of a union.
///
/// u: The union.
///
/// idx: The field to return. If -1, return whatever field is currently set.
///
/// \hlt_c
///
/// Returns: Returns a pointer to the value of the field with index \a idx if
/// it's set, and null otherwise.
extern void* hlt_union_get(hlt_union* u, int64_t idx, hlt_exception** excpt,
                           hlt_execution_context* ctx);

/// Returns the type and other meta information about the a field in a union.
///
/// type: The type of the union
///
/// u: A pointer to the union. Can be null if idx is -1.
///
/// idx: The zero-based index of the field, in order listed in the type's
/// definition. Set to -1 to return the information for the currently set
/// field.
///
/// \hlt_c
///
/// Returns: A pointer to the meta information. All pointers in there must be
/// left untouched, ownership is *not* passed to caller. If idx is -1 and no
/// field is currently set, the type information in there will be null.
extern hlt_union_field hlt_union_get_type(const hlt_type_info* type, hlt_union* u, int idx,
                                          hlt_exception** excpt, hlt_execution_context* ctx);

/// Converts a HILTI union into a HILTI string.
///
/// \hlt_to_string
extern hlt_string hlt_union_to_string(const hlt_type_info* type, void* obj, int32_t options,
                                      __hlt_pointer_stack* seen, hlt_exception** excpt,
                                      hlt_execution_context* ctx);

#endif
