
#include <stdio.h>
#include <assert.h>

#include "rtti.h"
#include "tuple.h"
#include "string_.h"

// One entry in the type parameter array.
struct _element {
    const char* name;
    int16_t offset;
};

// Generic version working with all tuple types.
void hlt_tuple_dtor(hlt_type_info* ti, void* obj, hlt_execution_context* ctx)
{
    hlt_type_info** types = (hlt_type_info**) &ti->type_params;
    struct _element* elements = (struct _element *)ti->aux;

    for ( int i = 0; i < ti->num_params; i++ ) {
        __hlt_object_dtor(types[i], obj + elements[i].offset, "tuple-dtor", ctx);
    }
}

// Generic version working with all tuple types.
void hlt_tuple_cctor(hlt_type_info* ti, void* obj, hlt_execution_context* ctx)
{
    hlt_type_info** types = (hlt_type_info**) &ti->type_params;
    struct _element* elements = (struct _element *)ti->aux;

    for ( int i = 0; i < ti->num_params; i++ ) {
        __hlt_object_cctor(types[i], obj + elements[i].offset, "tuple-dtor", ctx);
    }
}

// Generic version working with all tuple types.
void hlt_tuple_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* src = (const char*)srcp;
    char* dst = (char*)dstp;

    if ( ! src ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0, ctx);
        return;
    }

    hlt_type_info** types = (hlt_type_info**) &ti->type_params;
    struct _element* elements = (struct _element *)ti->aux;

    for ( int i = 0; i < ti->num_params; i++ ) {
        int16_t offset = elements[i].offset;
        __hlt_clone(dst + offset, types[i], src + offset, cstate, excpt, ctx);
        if ( hlt_check_exception(excpt) )
            return;
    }
}

hlt_string hlt_tuple_to_string(const hlt_type_info* type, void* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string postfix = hlt_string_from_asciiz(")", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(",", excpt, ctx);
    hlt_string equal = hlt_string_from_asciiz("=", excpt, ctx);

    hlt_string result = 0;

    assert(type->type == HLT_TYPE_TUPLE);

    struct _element* elements = (struct _element *)type->aux;

    int i;
    hlt_string t = 0;
    hlt_string s = hlt_string_from_asciiz("(", excpt, ctx);

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {

        const char* name = elements[i].name;

        if ( name && *name ) {
            hlt_string hname = hlt_string_from_asciiz(name, excpt, ctx);
            s = hlt_string_concat(s, hname, excpt, ctx);
            s = hlt_string_concat(s, equal, excpt, ctx);
        }

        hlt_string t = __hlt_object_to_string(types[i], obj + elements[i].offset, 0, seen, excpt, ctx);
        s = hlt_string_concat(s, t, excpt, ctx);

        if ( i < type->num_params - 1 )
            s = hlt_string_concat(s, separator, excpt, ctx);

        if ( hlt_check_exception(excpt) )
            return 0;
    }

    return hlt_string_concat(s, postfix, excpt, ctx);
}

hlt_hash hlt_tuple_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TUPLE);

    hlt_hash hash = 0;

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    struct _element* elements = (struct _element *)type->aux;

    for ( int i = 0; i < type->num_params; i++ ) {
        assert(types[i]->hash);
        hash += (*types[i]->hash)(types[i], obj + elements[i].offset, excpt, ctx);
        hash *= 2147483647;
    }

    return hash;
}

int8_t hlt_tuple_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type1->type == HLT_TYPE_TUPLE);
    assert(type1->type == type2->type);

    hlt_type_info** types = (hlt_type_info**) &type1->type_params;
    struct _element* elements = (struct _element *)type1->aux;

    for ( int i = 0; i < type1->num_params; i++ ) {
        assert(types[i]->equal);
        if ( ! (*types[i]->equal)(types[i], obj1 + elements[i].offset, types[i], obj2 + elements[i].offset, excpt, ctx) )
            return 0;
    }

    return 1;
}

int hlt_tuple_length(const hlt_type_info* type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( type->type != HLT_TYPE_TUPLE ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

    return type->num_params;
}

void* hlt_tuple_get(const hlt_type_info* type, void *obj, int index, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( type->type != HLT_TYPE_TUPLE ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

    if ( index < 0 || index >= type->num_params ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return 0;
    }

    struct _element* elements = (struct _element *)type->aux;
    return obj + elements[index].offset;
}

hlt_tuple_element hlt_tuple_get_type(const hlt_type_info* type, int index, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_tuple_element e;

    if ( type->type != HLT_TYPE_TUPLE ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return e;
    }

    if ( index < 0 || index >= type->num_params ) {
        hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
        return e;
    }

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    struct _element* elements = (struct _element *)type->aux;

    e.name = elements[index].name;
    e.type = types[index];
    return e;
}
