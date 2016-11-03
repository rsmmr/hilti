//
// Run-time information about HILTI types.
//

#ifndef LIBHILTI_RTTI_H
#define LIBHILTI_RTTI_H

#include <stdint.h>

#include "types.h"

// Unique id values to identify a type.
#define HLT_TYPE_ERROR 0
#define HLT_TYPE_INTEGER 1
#define HLT_TYPE_DOUBLE 2
#define HLT_TYPE_BOOL 3
#define HLT_TYPE_STRING 4
#define HLT_TYPE_TUPLE 5
#define HLT_TYPE_REF 6
#define HLT_TYPE_STRUCT 7
#define HLT_TYPE_CHANNEL 8
#define HLT_TYPE_BYTES 9
#define HLT_TYPE_ENUM 10
#define HLT_TYPE_ENUM_LABEL 11
#define HLT_TYPE_ADDR 12
#define HLT_TYPE_PORT 13
#define HLT_TYPE_OVERLAY 14
#define HLT_TYPE_VECTOR 15
#define HLT_TYPE_LIST 16
#define HLT_TYPE_NET 17
#define HLT_TYPE_REGEXP 18
#define HLT_TYPE_BITSET 19
#define HLT_TYPE_EXCEPTION 20
#define HLT_TYPE_CONTINUATION 21
#define HLT_TYPE_CADDR 22
#define HLT_TYPE_LABEL 23
#define HLT_TYPE_ANY 24
#define HLT_TYPE_TIMER 25
#define HLT_TYPE_TIMER_MGR 26
#define HLT_TYPE_PKTSRC 27
#define HLT_TYPE_HOOK 28
#define HLT_TYPE_CAPTURED_HOOK 29
#define HLT_TYPE_FILE 30
#define HLT_TYPE_CALLABLE 31
#define HLT_TYPE_TIME 32
#define HLT_TYPE_INTERVAL 33
#define HLT_TYPE_CONTEXT 34
#define HLT_TYPE_SCOPE 35
#define HLT_TYPE_MATCH_TOKEN_STATE 36
#define HLT_TYPE_VOID 37
#define HLT_TYPE_BYTES_DATA 38
#define HLT_TYPE_BYTES_CHUNK 39
#define HLT_TYPE_CLASSIFIER 40
#define HLT_TYPE_IOSOURCE 41
#define HLT_TYPE_MAP 42
#define HLT_TYPE_SET 44
#define HLT_TYPE_LIST_NODE 45
#define HLT_TYPE_MAP_TIMER_COOKIE 46
#define HLT_TYPE_SET_TIMER_COOKIE 47
#define HLT_TYPE_UNSET 48
#define HLT_TYPE_FUNCTION 49
#define HLT_TYPE_UNION 50

#define HLT_TYPE_ITERATOR_BYTES 100
#define HLT_TYPE_ITERATOR_VECTOR 101
#define HLT_TYPE_ITERATOR_LIST 102
#define HLT_TYPE_ITERATOR_SET 103
#define HLT_TYPE_ITERATOR_MAP 104
#define HLT_TYPE_ITERATOR_IOSRC 105

#define HLT_TYPE_EXTERN_SPICY 10000 // Space for Spicy's types. FIXME: Better way to do this?

// Default context for numeric operations.
#define HLT_CONVERT_NONE 0x00     // Not given, implies doing signed operations.
#define HLT_CONVERT_UNSIGNED 0x01 // Numeric operations assume an unsigned context.

// End marker for a pointer map.
#define HLT_PTR_MAP_END 0xffff

struct __hlt_type_info {
    /// The type's HLT_TYPE_* id.
    int16_t type;

    /// The size of an instance in bytes. This corresponds to the size needed
    /// on the stack for storing an instance of this type; for a garbage
    /// collected type that's the size of the reference, not the object.
    int16_t size;

    /// For garbage collected objects, the size of an instance in bytes as
    /// it's stored on the heap. Note that not for all types this can be
    /// determined statically, and hence it's only provided for types where
    /// the compiler is explicitly told to do so; otherwise it's zero.
    int16_t object_size;

    /// A readable version of the type's name.
    const char* tag;

    /// Number of type parameters.
    int16_t num_params;

    /// True if instances of this type are garbage collected.
    int16_t gc;

    /// True if this type is atomic in the sense that it doesn't refer or
    /// contain to any other values. Being atomic implies that it can be
    /// copied and cloned with a bitwise copy, and that it won't need a cctor
    /// or dtor.
    int16_t atomic;

    /// Type-specific information; null if not used.
    void* aux;

    /// Pointer map, storing the offsets witin instances that point to
    /// further garbage collected memory. The end is marked by an entry
    /// HLT_PTR_MAP_END.
    ///
    /// Note this is not yet used, but still already generated.
    int16_t* ptr_map;

    // Converters for the value into different types. 'type' is the type
    // information for the type being converted. 'obj' is a pointer to the
    // value stored with the C type as HILTI uses normally for values of that
    // type. 'options' is a bitmask of HLT_CONVERT_* options. 'expt' can be
    // set to raise an exception.
    struct __hlt_string* (*to_string)(const hlt_type_info* type, const void* obj, int32_t options,
                                      __hlt_pointer_stack* seen, hlt_exception** expt,
                                      hlt_execution_context* ctx);
    int64_t (*to_int64)(const hlt_type_info* type, const void* obj, int32_t options,
                        hlt_exception** expt, hlt_execution_context* ctx);
    double (*to_double)(const hlt_type_info* type, const void* obj, int32_t options,
                        hlt_exception** expt, hlt_execution_context* ctx);

    // Calculates a hash of the value. If not given, the default is hash as
    // many bytes as size specifies. Note that the excpt and ctx arguments
    // will not be used and will always be null.
    hlt_hash (*hash)(const hlt_type_info* type, const void* obj, hlt_exception** expt,
                     hlt_execution_context* ctx);

    /// Compares to values for equality. If not given, the default is to
    /// compare as many bytes as size specified.  Note that the excpt and ctx
    /// argument will not be used and will always be null.
    int8_t (*equal)(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2,
                    const void* obj2, hlt_exception** expt, hlt_execution_context* ctx);

    /// If this is a type that a "yield" can block for until a resource
    /// becomes available, a function returning hlt_thread_mgr_blockable
    /// object for an instance.
    __hlt_thread_mgr_blockable* (*blockable)(const hlt_type_info* type, const void* obj,
                                             hlt_exception** expt, hlt_execution_context* ctx);

    /// If this is a garbage collected type, a function that will be called
    /// if an objects reference count went down to zero and its about to be
    /// deleted. Can be null if no further cleanup is needed. The function
    /// must not fail.
    void (*dtor)(const struct __hlt_type_info* ti, void* obj, hlt_execution_context* ctx);

    /// XXX
    void (*obj_dtor)(const struct __hlt_type_info* ti, void* obj, hlt_execution_context* ctx);

    /// If this is a value type, a function that will be called if an
    /// instance has been copied. The function is called after a bitwise copy
    /// has been made, but before any further operation is executed that
    /// involves it.
    void (*cctor)(const struct __hlt_type_info* ti, void* obj, hlt_execution_context* ctx);

    /// Initializes a previously allocated instance with a deep-copy of a
    /// non-atomic value. \a obj is a pointer to the source object of type \a
    /// ti, and \a dst a pointer to newly allocated object that needs
    /// initialization. \a cstate is an opaque parameter that needs to be
    /// passed on to \a __hlt_clone() when other objects needs to be cloned
    /// recursively.
    ///
    /// This will be ignored for atomic types and should be set to null in
    /// that case.
    void (*clone_init)(void* dstp, const hlt_type_info* ti, const void* srcp,
                       __hlt_clone_state* cstate, hlt_exception** excpt,
                       hlt_execution_context* ctx);

    /// If this is a garbage collected type, a function that allocates a new
    /// instance of a type in preparation for deep-copying it, yet without
    /// yet initializing it (must be null for atomic types). clone_init()
    /// will be called afterwards and must be defined as well. \a obj is the
    /// source object that's about to be cloned, of type \a ti. \a cstate is
    /// an opaque parameter that needs to be passed on to \a __hlt_clone()
    /// when other objects needs to be cloned recursively. Returns the newly
    /// allocated instance.
    ///
    /// This will be ignored for non gc types and should be set to null in
    /// that case.
    void* (*clone_alloc)(const hlt_type_info* ti, const void* srcp, __hlt_clone_state* cstate,
                         hlt_exception** excpt, hlt_execution_context* ctx);

    /// A host application may associate further custom data with a type. If
    /// so, this will point to the start of that data; it's null otherwise.
    void* hostapp_value;

    /// If hostapp_expr is set, the type information for the value.
    const hlt_type_info* hostapp_type;

    // Type-parameters start here. The format is type-specific. They are
    // followed by further custom data that a host application may add;
    // "host_data" points to the start of that.
    char type_params[];
};

/// Macro to define type information for an internal garbage collected type.
#define __HLT_RTTI_GC_TYPE(id, type)                                                               \
    void id##_dtor(hlt_type_info*, id*, hlt_execution_context*);                                   \
    const hlt_type_info hlt_type_info_##id = {                                                     \
        type, sizeof(id), 0, #id, 0, 1, 0, 0, (int16_t*)-1, 0, 0, 0, 0, 0, 0,                      \
        (void (*)(const struct __hlt_type_info*, void*,                                            \
                  hlt_execution_context* ctx))__hlt_object_unref,                                  \
        (void (*)(const struct __hlt_type_info*, void*, hlt_execution_context* ctx))id##_dtor,     \
        (void (*)(const struct __hlt_type_info*, void*,                                            \
                  hlt_execution_context* ctx))__hlt_object_ref,                                    \
    }

#define __HLT_DECLARE_RTTI_GC_TYPE(id) extern const hlt_type_info hlt_type_info_##id

/// Returns true if the two type informations refer to equivalent types.
int8_t __hlt_type_equal(const hlt_type_info* ti1, const hlt_type_info* ti2);

// Include prototypes for compiler-generated type information.
#include "type-info.h"

#endif
