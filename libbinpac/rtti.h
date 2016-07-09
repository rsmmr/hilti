
#ifndef LIBBINPAC_RTTI_H
#define LIBBINPAC_RTTI_H

#include <libhilti/rtti.h>

// Define type constants for HILTI's RTTI system.
#define HLT_TYPE_BINPAC_FILTER (HLT_TYPE_EXTERN_BINPAC + 1)
#define HLT_TYPE_BINPAC_SINK (HLT_TYPE_EXTERN_BINPAC + 2)
#define HLT_TYPE_BINPAC_MIME_PARSER (HLT_TYPE_EXTERN_BINPAC + 3)

typedef uint64_t binpac_type_id;
typedef uint64_t binpac_unit_item_kind;

// Unique id values to identify a BinPAC++ type. These values go into the
// hostapp_valyue information of the HILTI RTTI information.
#define BINPAC_TYPE_NONE 0
#define BINPAC_TYPE_UNSET 1
#define BINPAC_TYPE_MODULE 2
#define BINPAC_TYPE_VOID 3
#define BINPAC_TYPE_STRING 4
#define BINPAC_TYPE_ADDRESS 5
#define BINPAC_TYPE_NETWORK 6
#define BINPAC_TYPE_PORT 7
#define BINPAC_TYPE_BITSET 8
#define BINPAC_TYPE_ENUM 9
#define BINPAC_TYPE_CADDR 10
#define BINPAC_TYPE_DOUBLE 11
#define BINPAC_TYPE_BOOL 12
#define BINPAC_TYPE_INTERVAL 13
#define BINPAC_TYPE_TIME 14
#define BINPAC_TYPE_BITS 15
#define BINPAC_TYPE_TUPLE 17
#define BINPAC_TYPE_TYPETYPE 18
#define BINPAC_TYPE_EXCEPTION 19
#define BINPAC_TYPE_FUNCTION 20
#define BINPAC_TYPE_HOOK 21
#define BINPAC_TYPE_BYTES 22
#define BINPAC_TYPE_FILE 23
#define BINPAC_TYPE_LIST 24
#define BINPAC_TYPE_VECTOR 25
#define BINPAC_TYPE_SET 26
#define BINPAC_TYPE_MAP 27
#define BINPAC_TYPE_REGEXP 28
#define BINPAC_TYPE_TIMERMGR 29
#define BINPAC_TYPE_TIMER 30
#define BINPAC_TYPE_UNIT 31
#define BINPAC_TYPE_SINK 32
#define BINPAC_TYPE_EMBEDDED_OBJECT 33
#define BINPAC_TYPE_MARK 34
#define BINPAC_TYPE_OPTIONAL 35
#define BINPAC_TYPE_BITFIELD 36
#define BINPAC_TYPE_INTEGER_UNSIGNED 37
#define BINPAC_TYPE_INTEGER_SIGNED 38

#define BINPAC_TYPE_ITERATOR_BYTES 100
#define BINPAC_TYPE_ITERATOR_MAP 101
#define BINPAC_TYPE_ITERATOR_SET 102
#define BINPAC_TYPE_ITERATOR_VECTOR 103
#define BINPAC_TYPE_ITERATOR_LIST 104

#define BINPAC_UNIT_ITEM_NONE 0
#define BINPAC_UNIT_ITEM_FIELD 1
#define BINPAC_UNIT_ITEM_VARIABLE 2

/// When iterating over the fields of a unit instance, include also those
/// that are not set. The default is to skip unset fields.
#define BINPAC_UNIT_ITERATE_INCLUDE_ALL 1

/// Returns the BINPAC_TYPE_* ID associated with a HILTI type. Returns
/// BINPAC_TYPE_ERROR if none.
binpac_type_id binpac_type_get_id(const hlt_type_info* type, hlt_exception** excpt,
                                  hlt_execution_context* ctx);

/// Meta-information about a unit item in the C representation.
struct __binpac_unit_item {
    const char* name;           ///< The name of the item. Empty string for anonymous items.
    const hlt_type_info* type;  ///< The type of the item's HILTI value.
    binpac_unit_item_kind kind; ///< One of the BINPAC_UNIT_ITEM_* constants.
    int8_t hide;                ///< True if the item has been marked with &hide.

    /// A pointer to the item's HILTI value. Null if the item is not set, or
    /// no unit instance has been specified. Note that for a type
    /// reference<T> this will be a pointer to the *reference*; not the
    /// object. You need to dereference that once to get the object.
    void* value;
};

typedef struct __binpac_unit_item binpac_unit_item;

typedef uint64_t binpac_unit_cookie;

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
/// flags: A combination of BINPAC_UNIT_ITERATE_* flags. Use 0 for default
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
extern binpac_unit_cookie binpac_unit_iterate(binpac_unit_item* dst, const hlt_type_info* type,
                                              void* unit, int flags, binpac_unit_cookie cookie,
                                              hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the element type of a list.
extern const hlt_type_info* binpac_list_element_type(const hlt_type_info* type,
                                                     hlt_exception** excpt,
                                                     hlt_execution_context* ctx);

/// Returns the element type of a set.
extern const hlt_type_info* binpac_set_element_type(const hlt_type_info* type,
                                                    hlt_exception** excpt,
                                                    hlt_execution_context* ctx);

/// Returns the element type of a vector.
extern const hlt_type_info* binpac_vector_element_type(const hlt_type_info* type,
                                                       hlt_exception** excpt,
                                                       hlt_execution_context* ctx);

/// Returns the key type of a map.
extern const hlt_type_info* binpac_map_key_type(const hlt_type_info* type, hlt_exception** excpt,
                                                hlt_execution_context* ctx);

/// Returns the value type of a map.
extern const hlt_type_info* binpac_map_value_type(const hlt_type_info* type, hlt_exception** excpt,
                                                  hlt_execution_context* ctx);

/// Returns the wrapped type T of an \c optional<T> type.
extern const hlt_type_info* binpac_optional_type(const hlt_type_info* type, hlt_exception** excpt,
                                                 hlt_execution_context* ctx);

/// Returns the type embedded insided an object<T> type.
extern const hlt_type_info* binpac_embedded_type(const hlt_type_info* type, hlt_exception** excpt,
                                                 hlt_execution_context* ctx);

/// Returns the width of an integer type.
extern int binpac_integer_witdh(const hlt_type_info* type, hlt_exception** excpt,
                                hlt_execution_context* ctx);

/// Returns true if an integer type is signed, false if unsigned.
extern int8_t binpac_integer_signed(const hlt_type_info* type, hlt_exception** excpt,
                                    hlt_execution_context* ctx);


#endif
