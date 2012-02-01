
#include <stdio.h>
#include <assert.h>

#include "rtti.h"
#include "string_.h"

hlt_string hlt_tuple_to_string(const hlt_type_info* type, const char* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string prefix = hlt_string_from_asciiz("(", excpt, ctx);
    hlt_string postfix = hlt_string_from_asciiz(")", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(",", excpt, ctx);

    hlt_string result = 0;

    assert(type->type == HLT_TYPE_TUPLE);

    int16_t* offsets = (int16_t *)type->aux;

    int i;
    hlt_string t = 0;
    hlt_string s = hlt_string_from_asciiz("", excpt, ctx);

    s = hlt_string_concat(s, prefix, excpt, ctx);
    if ( *excpt )
        goto error;

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {
        hlt_string t;

        if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj + offsets[i] , 0, excpt, ctx);
            if ( *excpt )
                goto error;
        }
        else
            // No format function.
            t = hlt_string_from_asciiz(types[i]->tag, excpt, ctx);

        s = hlt_string_concat(s, t, excpt, ctx);
        if ( *excpt )
            goto error;

        hlt_string_unref(t);

        if ( i < type->num_params - 1 ) {
            s = hlt_string_concat(s, separator, excpt, ctx);
            if ( *excpt )
                goto error;
        }

    }

    result = hlt_string_concat(s, postfix, excpt, ctx);

error:
    if ( ! result )
        hlt_string_unref(s);

    hlt_string_unref(s);
    hlt_string_unref(t);

    hlt_string_unref(prefix);
    hlt_string_unref(postfix);
    hlt_string_unref(separator);

    return result;
}

hlt_hash hlt_tuple_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_TUPLE);

    hlt_hash hash = 0;

    int16_t* offsets = (int16_t *)type->aux;
    hlt_type_info** types = (hlt_type_info**) &type->type_params;

    for ( int i = 0; i < type->num_params; i++ ) {
        assert(types[i]->hash);
        hash += (*types[i]->hash)(types[i], obj + offsets[i], excpt, ctx);
        hash *= 2147483647;
    }

    return hash;
}

int8_t hlt_tuple_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type1->type == HLT_TYPE_TUPLE);
    assert(type1->type == type2->type);

    int16_t* offsets = (int16_t *)type1->aux;
    hlt_type_info** types = (hlt_type_info**) &type1->type_params;

    for ( int i = 0; i < type1->num_params; i++ ) {
        assert(types[i]->equal);
        if ( ! (*types[i]->equal)(types[i], obj1 + offsets[i], types[i], obj2 + offsets[i], excpt, ctx) )
            return 0;
    }

    return 1;
}
