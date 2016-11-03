
#ifndef LIBSPICY_RTTI_H
#define LIBSPICY_RTTI_H

#include <libhilti/rtti.h>

// Define type constants for HILTI's RTTI system.
#define HLT_TYPE_SPICY_FILTER (HLT_TYPE_EXTERN_SPICY + 1)
#define HLT_TYPE_SPICY_SINK (HLT_TYPE_EXTERN_SPICY + 2)
#define HLT_TYPE_SPICY_MIME_PARSER (HLT_TYPE_EXTERN_SPICY + 3)

typedef uint64_t spicy_type_id;
typedef uint64_t spicy_unit_item_kind;

// Unique id values to identify a Spicy type. These values go into the
// hostapp_valyue information of the HILTI RTTI information.
#define SPICY_TYPE_NONE 0
#define SPICY_TYPE_UNSET 1
#define SPICY_TYPE_MODULE 2
#define SPICY_TYPE_VOID 3
#define SPICY_TYPE_STRING 4
#define SPICY_TYPE_ADDRESS 5
#define SPICY_TYPE_NETWORK 6
#define SPICY_TYPE_PORT 7
#define SPICY_TYPE_BITSET 8
#define SPICY_TYPE_ENUM 9
#define SPICY_TYPE_CADDR 10
#define SPICY_TYPE_DOUBLE 11
#define SPICY_TYPE_BOOL 12
#define SPICY_TYPE_INTERVAL 13
#define SPICY_TYPE_TIME 14
#define SPICY_TYPE_BITS 15
#define SPICY_TYPE_TUPLE 17
#define SPICY_TYPE_TYPETYPE 18
#define SPICY_TYPE_EXCEPTION 19
#define SPICY_TYPE_FUNCTION 20
#define SPICY_TYPE_HOOK 21
#define SPICY_TYPE_BYTES 22
#define SPICY_TYPE_FILE 23
#define SPICY_TYPE_LIST 24
#define SPICY_TYPE_VECTOR 25
#define SPICY_TYPE_SET 26
#define SPICY_TYPE_MAP 27
#define SPICY_TYPE_REGEXP 28
#define SPICY_TYPE_TIMERMGR 29
#define SPICY_TYPE_TIMER 30
#define SPICY_TYPE_UNIT 31
#define SPICY_TYPE_SINK 32
#define SPICY_TYPE_EMBEDDED_OBJECT 33
#define SPICY_TYPE_MARK 34
#define SPICY_TYPE_OPTIONAL 35
#define SPICY_TYPE_BITFIELD 36
#define SPICY_TYPE_INTEGER_UNSIGNED 37
#define SPICY_TYPE_INTEGER_SIGNED 38

#define SPICY_TYPE_ITERATOR_BYTES 100
#define SPICY_TYPE_ITERATOR_MAP 101
#define SPICY_TYPE_ITERATOR_SET 102
#define SPICY_TYPE_ITERATOR_VECTOR 103
#define SPICY_TYPE_ITERATOR_LIST 104

#define SPICY_UNIT_ITEM_NONE 0
#define SPICY_UNIT_ITEM_FIELD 1
#define SPICY_UNIT_ITEM_VARIABLE 2

/// When iterating over the fields of a unit instance, include also those
/// that are not set. The default is to skip unset fields.
#define SPICY_UNIT_ITERATE_INCLUDE_ALL 1

/// Returns the SPICY_TYPE_* ID associated with a HILTI type. Returns
/// SPICY_TYPE_ERROR if none.
spicy_type_id spicy_type_get_id(const hlt_type_info* type, hlt_exception** excpt,
                                  hlt_execution_context* ctx);

/// Meta-information about a unit item in the C representation.
struct __spicy_unit_item {
    const char* name;           ///< The name of the item. Empty string for anonymous items.
    const hlt_type_info* type;  ///< The type of the item's HILTI value.
    spicy_unit_item_kind kind; ///< One of the SPICY_UNIT_ITEM_* constants.
    int8_t hide;                ///< True if the item has been marked with &hide.

    /// A pointer to the item's HILTI value. Null if the item is not set, or
    /// no unit instance has been specified. Note that for a type
    /// reference<T> this will be a pointer to the *reference*; not the
    /// object. You need to dereference that once to get the object.
    void* value;
};

typedef struct __spicy_unit_item spicy_unit_item;

typedef uint64_t spicy_unit_cookie;

/// Iterates through the items of a unit.
///
/// dst: A pointer to a struct that the function will fill out with the next item.
///
/// type: The type of the unit. On successive calls, you must always pass in
/// the same value here.
///
/// unit: An instance of the unit. If given, iteration will set the \a value
/// element of \a dst to the corresponding item value (if it's set). If null,
/// \a value will always be left null. On successive calls, you must always
/// pass in the same value here.
///
/// flags: A combination of SPICY_UNIT_ITERATE_* flags. Use 0 for default
/// behaviour.
///
/// cookie: Internal cookie for keeping track of the iteration. For the first
/// call, this must be null. Subsequently, always pass in the value returned
/// by the previous call.
///
/// \hltc
///
/// Returns: Cookie for next call, or 0 if end has been reached. In the
/// latter case, \a dst won't have been changed.
extern spicy_unit_cookie spicy_unit_iterate(spicy_unit_item* dst, const hlt_type_info* type,
                                              void* unit, int flags, spicy_unit_cookie cookie,
                                              hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the element type of a list.
extern const hlt_type_info* spicy_list_element_type(const hlt_type_info* type,
                                                     hlt_exception** excpt,
                                                     hlt_execution_context* ctx);

/// Returns the element type of a set.
extern const hlt_type_info* spicy_set_element_type(const hlt_type_info* type,
                                                    hlt_exception** excpt,
                                                    hlt_execution_context* ctx);

/// Returns the element type of a vector.
extern const hlt_type_info* spicy_vector_element_type(const hlt_type_info* type,
                                                       hlt_exception** excpt,
                                                       hlt_execution_context* ctx);

/// Returns the key type of a map.
extern const hlt_type_info* spicy_map_key_type(const hlt_type_info* type, hlt_exception** excpt,
                                                hlt_execution_context* ctx);

/// Returns the value type of a map.
extern const hlt_type_info* spicy_map_value_type(const hlt_type_info* type, hlt_exception** excpt,
                                                  hlt_execution_context* ctx);

/// Returns the wrapped type T of an \c optional<T> type.
extern const hlt_type_info* spicy_optional_type(const hlt_type_info* type, hlt_exception** excpt,
                                                 hlt_execution_context* ctx);

/// Returns the type embedded insided an object<T> type.
extern const hlt_type_info* spicy_embedded_type(const hlt_type_info* type, hlt_exception** excpt,
                                                 hlt_execution_context* ctx);

/// Returns the width of an integer type.
extern int spicy_integer_witdh(const hlt_type_info* type, hlt_exception** excpt,
                                hlt_execution_context* ctx);

/// Returns true if an integer type is signed, false if unsigned.
extern int8_t spicy_integer_signed(const hlt_type_info* type, hlt_exception** excpt,
                                    hlt_execution_context* ctx);


#endif
