/* $Id$
 *
 * Support functions HILTI's struct data type.
 *
 */

#include "hilti.h"
#include "str.h"

#include <stdio.h>

static hlt_string_constant prefix = { 1, "<" };
static hlt_string_constant postfix = { 1, ">" };
static hlt_string_constant separator = { 2, ", " };
static hlt_string_constant equal = { 1, "=" };
static hlt_string_constant null = { 6, "<null>" };

// One entry in the type parameter array.
struct field {
    const char* field;
    int16_t offset;
};

hlt_string hlt_struct_to_string(const hlt_type_info* type, const void* obj, int32_t options, hlt_exception** excpt, hlt_execution_context* ctx)
{
    assert(type->type == HLT_TYPE_STRUCT);

    struct field* array = (struct field *)type->aux;

    obj = *((const char**)obj);

    if ( ! obj )
        return &null;

    uint32_t mask = *((uint32_t*)obj);

    int i;
    hlt_string s = hlt_string_from_asciiz("", excpt, ctx);

    s = hlt_string_concat(s, &prefix, excpt, ctx);
    if ( *excpt )
        return 0;

    hlt_type_info** types = (hlt_type_info**) &type->type_params;
    for ( i = 0; i < type->num_params; i++ ) {

        if ( array[i].field[0] && array[i].field[1] &&
             array[i].field[0] == '_' && array[i].field[1] == '_' )
            // Don't print internal names.
            continue;

        if ( i >  0 ) {
            s = hlt_string_concat(s, &separator, excpt, ctx);
            if ( *excpt )
                return 0;
        }

        hlt_string t;

        uint32_t is_set = (mask & (1 << i));

        if ( ! is_set )
            t = hlt_string_from_asciiz("(not set)", excpt, ctx);

        else if ( types[i]->to_string ) {
            t = (types[i]->to_string)(types[i], obj + array[i].offset, 0, excpt, ctx);
            if ( *excpt )
                return 0;
        }
        else
            // No format function.
            t = hlt_string_from_asciiz(types[i]->tag, excpt, ctx);

        hlt_string field_s = hlt_string_from_asciiz(array[i].field, excpt, ctx);

        s = hlt_string_concat(s, field_s, excpt, ctx);
        s = hlt_string_concat(s, &equal, excpt, ctx);
        s = hlt_string_concat(s, t, excpt, ctx);
        if ( *excpt )
            return 0;

    }

    return hlt_string_concat(s, &postfix, excpt, ctx);
}

hlt_hash hlt_struct_hash(const hlt_type_info* type, const void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_hash hash = 0;

    assert(type->type == HLT_TYPE_STRUCT);

    struct field* array = (struct field *)type->aux;

    obj = *((const char**)obj);

    if ( ! obj )
        return 0;

    uint32_t mask = *((uint32_t*)obj);

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
    assert(false); // This function is untested.

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

    uint32_t mask1 = *((uint32_t*)obj1);
    uint32_t mask2 = *((uint32_t*)obj2);

    if ( mask1 != mask2 )
        return false;

    hlt_type_info** types1 = (hlt_type_info**) &type1->type_params;
    hlt_type_info** types2 = (hlt_type_info**) &type2->type_params;

    if ( type1->num_params != type2->num_params )
        return false;

    for ( int i = 0; i < type1->num_params; i++ ) {

        if ( types1[i]->type != types2[i]->type )
            return false;

        uint32_t is_set = (mask1 & (1 << i));

        if ( is_set ) {
            assert(types1[i]->hash);
            assert(types2[i]->hash);

            if ( ! (types1[i]->equal)(types1[i], obj1 + array1[i].offset, types2[i], obj2 + array2[i].offset, excpt, ctx) )
                return false;
        }
    }
    return true;
}



