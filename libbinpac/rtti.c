
#include "rtti.h"

#include <libhilti/libhilti.h>

binpac_type_id binpac_type_get_id(const hlt_type_info* type, hlt_exception** excpt,
                                  hlt_execution_context* ctx)
{
    if ( type->hostapp_value ) {
        void* ptr = hlt_tuple_get(type->hostapp_type, type->hostapp_value, 0, excpt, ctx);
        return *((uint64_t*)ptr);
    }

    switch ( type->type ) {
    case HLT_TYPE_VOID:
        return BINPAC_TYPE_VOID;

    case HLT_TYPE_STRING:
        return BINPAC_TYPE_STRING;

    case HLT_TYPE_ADDR:
        return BINPAC_TYPE_ADDRESS;

    case HLT_TYPE_NET:
        return BINPAC_TYPE_NETWORK;

    case HLT_TYPE_PORT:
        return BINPAC_TYPE_PORT;

    case HLT_TYPE_BITSET:
        return BINPAC_TYPE_BITSET;

    case HLT_TYPE_ENUM:
        return BINPAC_TYPE_ENUM;

    case HLT_TYPE_CADDR:
        return BINPAC_TYPE_CADDR;

    case HLT_TYPE_DOUBLE:
        return BINPAC_TYPE_DOUBLE;

    case HLT_TYPE_BOOL:
        return BINPAC_TYPE_BOOL;

    case HLT_TYPE_INTERVAL:
        return BINPAC_TYPE_INTERVAL;

    case HLT_TYPE_TIME:
        return BINPAC_TYPE_TIME;

    case HLT_TYPE_TUPLE:
        return BINPAC_TYPE_TUPLE;

    case HLT_TYPE_EXCEPTION:
        return BINPAC_TYPE_EXCEPTION;

    case HLT_TYPE_FUNCTION:
        return BINPAC_TYPE_FUNCTION;

    case HLT_TYPE_HOOK:
        return BINPAC_TYPE_HOOK;

    case HLT_TYPE_BYTES:
        return BINPAC_TYPE_BYTES;

    case HLT_TYPE_FILE:
        return BINPAC_TYPE_FILE;

    case HLT_TYPE_LIST:
        return BINPAC_TYPE_LIST;

    case HLT_TYPE_VECTOR:
        return BINPAC_TYPE_VECTOR;

    case HLT_TYPE_SET:
        return BINPAC_TYPE_SET;

    case HLT_TYPE_MAP:
        return BINPAC_TYPE_MAP;

    case HLT_TYPE_REGEXP:
        return BINPAC_TYPE_REGEXP;

    case HLT_TYPE_TIMER_MGR:
        return BINPAC_TYPE_TIMERMGR;

    case HLT_TYPE_TIMER:
        return BINPAC_TYPE_TIMER;

    case HLT_TYPE_ITERATOR_SET:
        return BINPAC_TYPE_ITERATOR_SET;

    case HLT_TYPE_ITERATOR_MAP:
        return BINPAC_TYPE_ITERATOR_MAP;

    case HLT_TYPE_ITERATOR_VECTOR:
        return BINPAC_TYPE_ITERATOR_VECTOR;

    case HLT_TYPE_ITERATOR_LIST:
        return BINPAC_TYPE_ITERATOR_LIST;

    default:
        return BINPAC_TYPE_NONE;
    }
}

binpac_unit_cookie binpac_unit_iterate(binpac_unit_item* dst, const hlt_type_info* type, void* unit,
                                       int flags, binpac_unit_cookie idx, hlt_exception** excpt,
                                       hlt_execution_context* ctx)
{
    if ( ! idx )
        // First call.
        idx = 1; // +1, as zero is reserved as end marker.

    // Double check that the 1st tuple element gives the right type.
    binpac_type_id btype =
        *(binpac_type_id*)hlt_tuple_get(type->hostapp_type, type->hostapp_value, 0, excpt, ctx);

    if ( btype != BINPAC_TYPE_UNIT ) {
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
    // kind      - The item type as a BINPAC_UNIT_ITEM_* constant.
    // hide      - Boolean that's true if the &hide attribute is present.
    // path      - A tuple of int<64>s that describe the access path to the item from the top-level
    // unit.
    //
    binpac_unit_item_kind ikind = *(binpac_unit_item_kind*)hlt_tuple_get(tcur, cur, 0, excpt, ctx);
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
            fprintf(stderr, "unexpected type in binpac_unit_iterate %d/%s", ti->type, ti->tag);
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
