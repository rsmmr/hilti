/*
 *
 * Support functions HILTI's struct data type.
 *
 */


#include <stdio.h>
#include <assert.h>

#include "rtti.h"
#include "struct.h"
#include "string_.h"

#include <stdio.h>

// One entry in the type parameter array.
struct field {
    const char* field;
    int16_t offset;
};

// Generic version working with all struct types.
void hlt_struct_dtor(hlt_type_info* type, void* obj, const char* location)
{
    assert(type->type == HLT_TYPE_STRUCT);

    struct field* array = (struct field *)type->aux;

    obj = *((char**)obj);

    if ( ! obj )
        return;

    uint32_t mask = *((uint32_t*)(obj + sizeof(__hlt_gchdr)));

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( int i = 0; i < type->num_params; i++ ) {
        uint32_t is_set = (mask & (1 << i));

        if ( is_set )
            GC_DTOR_GENERIC(obj + array[i].offset, types[i]);
    }
}

hlt_string hlt_struct_to_string(const hlt_type_info* type, void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_STRUCT);

    struct field* array = (struct field *)type->aux;

    obj = *((char**)obj);

    if ( ! obj )
        return 0;

    uint32_t mask = *((uint32_t*)(obj + sizeof(__hlt_gchdr)));

    hlt_string separator = hlt_string_from_asciiz(", ", excpt, ctx);
    hlt_string equal = hlt_string_from_asciiz("=", excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("<", excpt, ctx);

    hlt_type_info** types = (hlt_type_info**) &type->type_params;

    for ( int i = 0; i < type->num_params; i++ ) {

        if ( array[i].field[0] && array[i].field[1] &&
             array[i].field[0] == '_' && array[i].field[1] == '_' )
            // Don't print internal names.
            continue;

        if ( i >  0 ) {
            GC_CCTOR(separator, hlt_string)
            s = hlt_string_concat_and_unref(s, separator, excpt, ctx);
        }

        hlt_string field_s = hlt_string_from_asciiz(array[i].field, excpt, ctx);
        s = hlt_string_concat_and_unref(s, field_s, excpt, ctx);

        GC_CCTOR(equal, hlt_string);
        s = hlt_string_concat_and_unref(s, equal, excpt, ctx);

        uint32_t is_set = (mask & (1 << i));

        if ( ! is_set ) {
            hlt_string not_set = hlt_string_from_asciiz("(not set)", excpt, ctx);
            s = hlt_string_concat_and_unref(s, not_set, excpt, ctx);
        }

        else if ( types[i]->to_string ) {
            hlt_string t = (types[i]->to_string)(types[i], obj + array[i].offset, 0, excpt, ctx);
            s = hlt_string_concat_and_unref(s, t, excpt, ctx);
        }

        else {
            // No format function.
            hlt_string t = hlt_string_from_asciiz(types[i]->tag, excpt, ctx);
            s = hlt_string_concat_and_unref(s, t, excpt, ctx);
        }

        if ( *excpt )
            goto error;
    }

    hlt_string postfix = hlt_string_from_asciiz(">", excpt, ctx);
    s = hlt_string_concat_and_unref(s, postfix, excpt, ctx);

    GC_DTOR(separator, hlt_string);
    GC_DTOR(equal, hlt_string);

    return s;

error:
    GC_DTOR(separator, hlt_string);
    GC_DTOR(equal, hlt_string);
    GC_DTOR(s, hlt_string);

    return 0;
}

hlt_hash hlt_struct_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_hash hash = 0;

    assert(type->type == HLT_TYPE_STRUCT);

    struct field* array = (struct field *)type->aux;

    obj = *((const char**)obj);

    if ( ! obj )
        return 0;

    uint32_t mask = *((uint32_t*)(obj + sizeof(__hlt_gchdr)));

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( int i = 0; i < type->num_params; i++ ) {

        uint32_t is_set = (mask & (1 << i));

        if ( is_set ) {
            assert(types[i]->hash);
            hlt_hash h = (types[i]->hash)(types[i], obj + array[i].offset, excpt, ctx);
            hash += h;
            hash *= 2654435761;
        }
        else
            hash += 8589935681;
    }

    return hash;
}

int8_t hlt_struct_equal(const hlt_type_info* type1, const void* obj1, const hlt_type_info* type2, const void* obj2, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type1->type == HLT_TYPE_STRUCT);
    assert(type1->type == type2->type);

    struct field* array1 = (struct field *)type1->aux;
    struct field* array2 = (struct field *)type2->aux;

    obj1 = *((const char**)obj1);
    obj2 = *((const char**)obj2);

    if ( ! obj1 )
        return !obj2;

    if ( ! obj2 )
        return !obj1;

    uint32_t mask1 = *((uint32_t*)(obj1 + sizeof(__hlt_gchdr)));
    uint32_t mask2 = *((uint32_t*)(obj2 + sizeof(__hlt_gchdr)));

    if ( mask1 != mask2 )
        return 0;

    hlt_type_info** types1 = (hlt_type_info**) &type1->type_params;
    hlt_type_info** types2 = (hlt_type_info**) &type2->type_params;

    if ( type1->num_params != type2->num_params )
        return 0;

    for ( int i = 0; i < type1->num_params; i++ ) {

        if ( types1[i]->type != types2[i]->type )
            return 0;

        uint32_t is_set = (mask1 & (1 << i));

        if ( is_set ) {
            assert(types1[i]->hash);
            assert(types2[i]->hash);

            if ( ! (types1[i]->equal)(types1[i], obj1 + array1[i].offset, types2[i], obj2 + array2[i].offset, excpt, ctx) )
                return 0;
        }
    }
   
    return 1;
}



