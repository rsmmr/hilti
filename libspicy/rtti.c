
#include "rtti.h"

#include <libhilti/libhilti.h>

spicy_type_id spicy_type_get_id(const hlt_type_info* type, hlt_exception** excpt,
                                hlt_execution_context* ctx)
{
    if ( type->hostapp_value ) {
        void* ptr = hlt_tuple_get(type->hostapp_type, type->hostapp_value, 0, excpt, ctx);
        return *((uint64_t*)ptr);
    }

    switch ( type->type ) {
    case HLT_TYPE_VOID:
        return SPICY_TYPE_VOID;

    case HLT_TYPE_STRING:
        return SPICY_TYPE_STRING;

    case HLT_TYPE_ADDR:
        return SPICY_TYPE_ADDRESS;

    case HLT_TYPE_NET:
        return SPICY_TYPE_NETWORK;

    case HLT_TYPE_PORT:
        return SPICY_TYPE_PORT;

    case HLT_TYPE_BITSET:
        return SPICY_TYPE_BITSET;

    case HLT_TYPE_ENUM:
        return SPICY_TYPE_ENUM;

    case HLT_TYPE_CADDR:
        return SPICY_TYPE_CADDR;

    case HLT_TYPE_DOUBLE:
        return SPICY_TYPE_DOUBLE;

    case HLT_TYPE_BOOL:
        return SPICY_TYPE_BOOL;

    case HLT_TYPE_INTERVAL:
        return SPICY_TYPE_INTERVAL;

    case HLT_TYPE_TIME:
        return SPICY_TYPE_TIME;

    case HLT_TYPE_TUPLE:
        return SPICY_TYPE_TUPLE;

    case HLT_TYPE_EXCEPTION:
        return SPICY_TYPE_EXCEPTION;

    case HLT_TYPE_FUNCTION:
        return SPICY_TYPE_FUNCTION;

    case HLT_TYPE_HOOK:
        return SPICY_TYPE_HOOK;

    case HLT_TYPE_BYTES:
        return SPICY_TYPE_BYTES;

    case HLT_TYPE_FILE:
        return SPICY_TYPE_FILE;

    case HLT_TYPE_LIST:
        return SPICY_TYPE_LIST;

    case HLT_TYPE_VECTOR:
        return SPICY_TYPE_VECTOR;

    case HLT_TYPE_SET:
        return SPICY_TYPE_SET;

    case HLT_TYPE_MAP:
        return SPICY_TYPE_MAP;

    case HLT_TYPE_REGEXP:
        return SPICY_TYPE_REGEXP;

    case HLT_TYPE_TIMER_MGR:
        return SPICY_TYPE_TIMERMGR;

    case HLT_TYPE_TIMER:
        return SPICY_TYPE_TIMER;

    case HLT_TYPE_ITERATOR_SET:
        return SPICY_TYPE_ITERATOR_SET;

    case HLT_TYPE_ITERATOR_MAP:
        return SPICY_TYPE_ITERATOR_MAP;

    case HLT_TYPE_ITERATOR_VECTOR:
        return SPICY_TYPE_ITERATOR_VECTOR;

    case HLT_TYPE_ITERATOR_LIST:
        return SPICY_TYPE_ITERATOR_LIST;

    default:
        return SPICY_TYPE_NONE;
    }
}

spicy_unit_cookie spicy_unit_iterate(spicy_unit_item* dst, const hlt_type_info* type, void* unit,
                                     int flags, spicy_unit_cookie idx, hlt_exception** excpt,
                                     hlt_execution_context* ctx)
{
    if ( ! idx )
        // First call.
        idx = 1; // +1, as zero is reserved as end marker.

    // Double check that the 1st tuple element gives the right type.
    spicy_type_id btype =
        *(spicy_type_id*)hlt_tuple_get(type->hostapp_type, type->hostapp_value, 0, excpt, ctx);

    if ( btype != SPICY_TYPE_UNIT ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

    // Get the 2nd tuple element, which lists all the items.
    void* items = hlt_tuple_get(type->hostapp_type, type->hostapp_value, 1, excpt, ctx);
    const hlt_type_info* titems = hlt_tuple_get_type(type->hostapp_type, 1, excpt, ctx).type;

    if ( titems->type != HLT_TYPE_TUPLE ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

#if 0
    fprintf(stderr, "\n| unit=%p unit_tag=%s idx=%lu item_value=%p\n",
            unit, type->tag, idx - 1, unit);
#endif

    //    if ( unit )
    //        hilti_print(type, &unit, 1, excpt, ctx);

    if ( idx > titems->num_params )
        return 0; // End reached.

    // Get the current item out of the 2nd tuple element.
    void* cur = hlt_tuple_get(titems, items, idx - 1, excpt, ctx);
    const hlt_type_info* tcur = hlt_tuple_get_type(titems, idx - 1, excpt, ctx).type;

    if ( tcur->type != HLT_TYPE_TUPLE ) {
        hlt_set_exception(excpt, &hlt_exception_type_error, 0, ctx);
        return 0;
    }

    // Each item is itself a tuple with the following elememts.
    //
    // kind      - The item type as a SPICY_UNIT_ITEM_* constant.
    // hide      - Boolean that's true if the &hide attribute is present.
    // path      - A tuple of int<64>s that describe the access path to the item from the top-level
    // unit.
    //
    spicy_unit_item_kind ikind = *(spicy_unit_item_kind*)hlt_tuple_get(tcur, cur, 0, excpt, ctx);
    int8_t ihide = *((uint8_t*)hlt_tuple_get(tcur, cur, 1, excpt, ctx));
    void* ipath = hlt_tuple_get(tcur, cur, 2, excpt, ctx);
    const hlt_type_info* ipath_type = hlt_tuple_get_type(tcur, 2, excpt, ctx).type;

    dst->kind = ikind;
    dst->hide = ihide;

    // Walk the path.
    //
    const hlt_type_info* ti = type;
    void* value = unit;
    const char* name = 0;

    for ( int i = 0; i < hlt_tuple_length(ipath_type, excpt, ctx); i++ ) {
        uint64_t j = *(uint64_t*)hlt_tuple_get(ipath_type, ipath, i, excpt, ctx);

        if ( ti->type == HLT_TYPE_STRUCT ) {
            if ( i > 0 && value )
                value = *(void**)value;

            if ( value && hlt_struct_is_set(ti, value, j, excpt, ctx) )
                value = hlt_struct_get(ti, value, j, excpt, ctx);
            else
                value = 0;

            hlt_struct_field sf = hlt_struct_get_type(ti, j, excpt, ctx);
            name = sf.name;
            ti = sf.type;
        }

        else if ( ti->type == HLT_TYPE_UNION ) {
            if ( value )
                value = hlt_union_get(value, j, excpt, ctx);

            hlt_union_field uf = hlt_union_get_type(ti, 0, j, excpt, ctx);
            name = uf.name;
            ti = uf.type;
        }

        else {
            fprintf(stderr, "unexpected type in spicy_unit_iterate %d/%s", ti->type, ti->tag);
            abort();
        }

#if 0
        fprintf(stderr, "  j=%lu unit=%p unit_tag=%s idx=%lu name=%s item_type=%d item_tag=%s item_value=%p kind=%d hide=%d\n",
                j, unit, type->tag, idx - 1, name, ti->type, ti->tag, value, (int)dst->kind, (int)dst->hide);
#endif
    }

    assert(name && ti);

    dst->name = name;
    dst->value = value;
    dst->type = ti;

    assert(ti && type);

#if 0
    fprintf(stderr, "unit=%p unit_tag=%s idx=%lu name=%s item_type=%d item_tag=%s item_value=%p kind=%d hide=%d\n",
            unit, type->tag, idx - 1, name, ti->type, ti->tag, dst->value, (int)dst->kind, (int)dst->hide);
#endif

    return idx + 1;
}
