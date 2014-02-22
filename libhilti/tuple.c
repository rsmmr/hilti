
#include <stdio.h>
#include <assert.h>

#include "rtti.h"
#include "string_.h"

// Generic version working with all tuple types.
void hlt_tuple_dtor(hlt_type_info* ti, void* obj, const char* location)
{
    hlt_type_info** types = (hlt_type_info**) &ti->type_params;
    int16_t* offsets = (int16_t *)ti->aux;

    for ( int i = 0; i < ti->num_params; i++ ) {
        __hlt_object_dtor(types[i], obj + offsets[i], location);
    }
}

// Generic version working with all tuple types.
void hlt_tuple_cctor(hlt_type_info* ti, void* obj, const char* location)
{
    hlt_type_info** types = (hlt_type_info**) &ti->type_params;
    int16_t* offsets = (int16_t *)ti->aux;

    for ( int i = 0; i < ti->num_params; i++ ) {
        __hlt_object_cctor(types[i], obj + offsets[i], location);
    }
}

// Generic version working with all tuple types.
void hlt_tuple_clone_init(void* dstp, const hlt_type_info* ti, void* srcp, __hlt_clone_state* cstate, hlt_exception** excpt, hlt_execution_context* ctx)
{
    const char* src = (const char*)srcp;
    char* dst = (char*)dstp;

    if ( ! src ) {
        hlt_set_exception(excpt, &hlt_exception_null_reference, 0);
        return;
    }

    hlt_type_info** types = (hlt_type_info**) &ti->type_params;
    int16_t* offsets = (int16_t *)ti->aux;

    for ( int i = 0; i < ti->num_params; i++ ) {
        int16_t offset = offsets[i];
        __hlt_clone(dst + offset, types[i], src + offset, cstate, excpt, ctx);
        if ( *excpt )
            return;
    }
}

hlt_string hlt_tuple_to_string(const hlt_type_info* type, const char* obj, int32_t options, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string postfix = hlt_string_from_asciiz(")", excpt, ctx);
    hlt_string separator = hlt_string_from_asciiz(",", excpt, ctx);

    hlt_string result = 0;

    assert(type->type == HLT_TYPE_TUPLE);

    int16_t* offsets = (int16_t *)type->aux;

    int i;
    hlt_string t = 0;
    hlt_string s = hlt_string_from_asciiz("(", excpt, ctx);

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {

        hlt_string t = hlt_object_to_string(types[i], obj + offsets[i], 0, seen, excpt, ctx);
        hlt_string tmp = s;
        s = hlt_string_concat(s, t, excpt, ctx);

        GC_DTOR(tmp, hlt_string);
        GC_DTOR(t, hlt_string);
        t = 0;

        if ( *excpt )
            goto error;

        if ( i < type->num_params - 1 ) {
            hlt_string tmp = s;
            s = hlt_string_concat(s, separator, excpt, ctx);
            GC_DTOR(tmp, hlt_string);

            if ( *excpt )
                goto error;
        }

    }

    result = hlt_string_concat(s, postfix, excpt, ctx);
    GC_DTOR(s, hlt_string);

error:
    if ( ! result )
        GC_DTOR(s, hlt_string);

    GC_DTOR(t, hlt_string);

    GC_DTOR(postfix, hlt_string);
    GC_DTOR(separator, hlt_string);

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
