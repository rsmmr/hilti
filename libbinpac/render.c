
#include "render.h"
#include "rtti.h"

#include "libhilti/libhilti.h"

static hlt_string _object_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);
static hlt_string _tuple_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx);

static hlt_string _addr_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _bitfield_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return _tuple_to_string(type, obj, seen, excpt, ctx);
}

static hlt_string _bool_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _bytes_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = hlt_object_to_string(type, obj, 0, excpt, ctx);

    hlt_string prefix = hlt_string_from_asciiz("b\"", excpt, ctx);
    s = hlt_string_concat(prefix, s, excpt, ctx);
    s = hlt_string_concat_asciiz(s, "\"", excpt, ctx);
    return s;
}

static hlt_string _double_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _embedded_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = hlt_object_to_string(type, obj, 0, excpt, ctx);
    hlt_string prefix = hlt_string_from_asciiz("object(", excpt, ctx);
    s = hlt_string_concat(prefix, s, excpt, ctx);
    s = hlt_string_concat_asciiz(s, ")", excpt, ctx);
    return s;
}

static hlt_string _enum_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _uint_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, HLT_CONVERT_UNSIGNED, excpt, ctx);
}

static hlt_string _sint_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _interval_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _iter_bytes_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _list_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list* list = *(hlt_list **)obj;
    const hlt_type_info* etype = hlt_list_element_type(type, excpt, ctx);

    hlt_iterator_list i = hlt_list_begin(list, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(list, excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("[", excpt, ctx);

    int first = 1;

    while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {
        void* ep = hlt_iterator_list_deref(i, excpt, ctx);
        hlt_string es = _object_to_string(etype, ep, seen, excpt, ctx);

        if ( ! first )
            s = hlt_string_concat_asciiz(s, ", ", excpt, ctx);

        s = hlt_string_concat(s, es, excpt, ctx);

        i = hlt_iterator_list_incr(i, excpt, ctx);
        first = 0;
    }

    s = hlt_string_concat_asciiz(s, "]", excpt, ctx);

    return s;
}

static hlt_string _map_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* map = *(hlt_map **)obj;
    const hlt_type_info* ktype = hlt_map_key_type(type, excpt, ctx);
    const hlt_type_info* vtype = hlt_map_value_type(type, excpt, ctx);

    hlt_iterator_map i = hlt_map_begin(map, excpt, ctx);
    hlt_iterator_map end = hlt_map_end(excpt, ctx);

    hlt_string sep = hlt_string_from_asciiz(": ", excpt, ctx);
    hlt_string s = hlt_string_from_asciiz("{", excpt, ctx);

    int first = 1;

    while ( ! hlt_iterator_map_eq(i, end, excpt, ctx) ) {
        void* kp = hlt_iterator_map_deref_key(i, excpt, ctx);
        void* vp = hlt_iterator_map_deref_value(i, excpt, ctx);
        hlt_string ks = _object_to_string(ktype, kp, seen, excpt, ctx);
        hlt_string vs = _object_to_string(vtype, vp, seen, excpt, ctx);

        if ( ! first )
            s = hlt_string_concat_asciiz(s, ", ", excpt, ctx);

        s = hlt_string_concat(s, ks, excpt, ctx);
        s = hlt_string_concat(s, sep, excpt, ctx);
        s = hlt_string_concat(s, vs, excpt, ctx);

        i = hlt_iterator_map_incr(i, excpt, ctx);
        first = 0;
    }

    s = hlt_string_concat_asciiz(s, "}", excpt, ctx);

    return s;
}

static hlt_string _optional_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_union* u = (hlt_union *)obj;

    void* v = hlt_union_get(u, -1, excpt, ctx);

    if ( ! v )
        return hlt_string_from_asciiz("(not set)", excpt, ctx);

    hlt_union_field f = hlt_union_get_type(type, u, -1, excpt, ctx);
    return hlt_object_to_string(f.type, v, 0, excpt, ctx);
}

static hlt_string _port_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _regexp_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _set_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* set = *(hlt_set **)obj;
    const hlt_type_info* etype = hlt_set_element_type(type, excpt, ctx);

    hlt_iterator_set i = hlt_set_begin(set, excpt, ctx);
    hlt_iterator_set end = hlt_set_end(excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("{", excpt, ctx);

    int first = 1;

    while ( ! hlt_iterator_set_eq(i, end, excpt, ctx) ) {
        void* ep = hlt_iterator_set_deref(i, excpt, ctx);
        hlt_string es = _object_to_string(etype, ep, seen, excpt, ctx);

        if ( ! first )
            s = hlt_string_concat_asciiz(s, ", ", excpt, ctx);

        s = hlt_string_concat(s, es, excpt, ctx);

        i = hlt_iterator_set_incr(i, excpt, ctx);
        first = 0;
    }

    s = hlt_string_concat_asciiz(s, "}", excpt, ctx);

    return s;
}

static hlt_string _sink_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_string_from_asciiz("<sink>", excpt, ctx);
}

static hlt_string _string_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
#if 1
    hlt_string s = *(hlt_string *)obj;
#else
    // TODO: We should differentiate if we are just printing out a string, in
    // which case we don't want quotes. Or if we are nested inside another
    // objects, in which we do.
    hlt_string prefix = hlt_string_from_asciiz("\"", excpt, ctx);
    s = hlt_string_concat(prefix, s, excpt, ctx);
    s = hlt_string_concat_asciiz(s, "\"", excpt, ctx);
#endif
    return s;
}

static hlt_string _time_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _tuple_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* tuple = obj;

    hlt_string s = hlt_string_from_asciiz("(", excpt, ctx);

    for ( int i = 0; i < hlt_tuple_length(type, excpt, ctx); i++ ) {
        void* ev = hlt_tuple_get(type, tuple, i, excpt, ctx);
        hlt_tuple_element et = hlt_tuple_get_type(type, i, excpt, ctx);

        if ( i > 0 )
            s = hlt_string_concat_asciiz(s, ", ", excpt, ctx);

        if ( et.name && *et.name ) {
            hlt_string n = hlt_string_from_asciiz(et.name, excpt, ctx);
            s = hlt_string_concat(s, n, excpt, ctx);
            s = hlt_string_concat_asciiz(s, "=", excpt, ctx);
        }

        hlt_string es = _object_to_string(et.type, ev, seen, excpt, ctx);
        s = hlt_string_concat(s, es, excpt, ctx);
    }

    s = hlt_string_concat_asciiz(s, ")", excpt, ctx);

    return s;
}

static hlt_string _unit_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* unit = *(void**)obj;

    if ( ! unit )
        return hlt_string_from_asciiz("(Null)", excpt, ctx);

    binpac_unit_cookie cookie = 0;
    binpac_unit_item item;

    int first = 1;

    hlt_string v;
    hlt_string s = hlt_string_from_asciiz("<", excpt, ctx);

    while ( (cookie = binpac_unit_iterate(&item, type, unit, 0, cookie, excpt, ctx)) ) {

        if ( ! item.value )
            // Don't print fields that aren't set.
            continue;

        if ( ! first )
            s = hlt_string_concat_asciiz(s, ", ", excpt, ctx);

        if ( item.name )
            s = hlt_string_concat_asciiz(s, item.name, excpt, ctx);
        else
            s = hlt_string_concat_asciiz(s, "<\?\?\?>", excpt, ctx);

        s = hlt_string_concat_asciiz(s, "=", excpt, ctx);

        v = item.value ? _object_to_string(item.type, item.value, seen, excpt, ctx)
                       : hlt_string_from_asciiz("(not set)", excpt, ctx);

        s  = hlt_string_concat(s, v, excpt, ctx);

        first = 0;
    }

    s = hlt_string_concat_asciiz(s, ">", excpt, ctx);

    return s;
}

static hlt_string _vector_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_vector* vector = *(hlt_vector **)obj;
    const hlt_type_info* etype = hlt_vector_element_type(type, excpt, ctx);

    hlt_iterator_vector i = hlt_vector_begin(vector, excpt, ctx);
    hlt_iterator_vector end = hlt_vector_end(vector, excpt, ctx);

    hlt_string s = hlt_string_from_asciiz("[", excpt, ctx);

    int first = 1;

    while ( ! hlt_iterator_vector_eq(i, end, excpt, ctx) ) {
        void* ep = hlt_iterator_vector_deref(i, excpt, ctx);
        hlt_string es = _object_to_string(etype, ep, seen, excpt, ctx);

        if ( ! first )
            s = hlt_string_concat_asciiz(s, ", ", excpt, ctx);

        s = hlt_string_concat(s, es, excpt, ctx);

        i = hlt_iterator_vector_incr(i, excpt, ctx);
        first = 0;
    }

    s = hlt_string_concat_asciiz(s, "]", excpt, ctx);

    return s;
}

static hlt_string _void_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    return hlt_object_to_string(type, obj, 0, excpt, ctx);
}

static hlt_string _object_to_string(const hlt_type_info* type, void* obj, __hlt_pointer_stack* seen, hlt_exception** excpt, hlt_execution_context* ctx)
{
    if ( type->gc ) {
        void *ptr = *(void **)obj;

        if ( __hlt_pointer_stack_lookup(seen, ptr) )
            return hlt_string_from_asciiz("[... recursion ...]", excpt, ctx);

        __hlt_pointer_stack_push_back(seen, ptr);
    }

    hlt_string s;

    binpac_type_id id = binpac_type_get_id(type, excpt, ctx);

    switch ( id ) {
     case BINPAC_TYPE_ADDRESS:
        s = _addr_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_BOOL:
        s = _bool_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_BITFIELD:
        s = _bitfield_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_BYTES:
        s = _bytes_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_DOUBLE:
        s = _double_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_EMBEDDED_OBJECT:
        s = _embedded_to_string(type, obj, seen, excpt, ctx);
        break
        ;
     case BINPAC_TYPE_ENUM:
        s = _enum_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_INTEGER_SIGNED:
        s = _sint_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_INTEGER_UNSIGNED:
        s = _uint_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_INTERVAL:
        s = _interval_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_ITERATOR_BYTES:
        s = _iter_bytes_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_LIST:
        s = _list_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_MAP:
        s = _map_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_OPTIONAL:
        s = _optional_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_PORT:
        s = _port_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_REGEXP:
        s = _regexp_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_SET:
        s = _set_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_SINK:
        s = _sink_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_STRING:
        s = _string_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_TIME:
        s = _time_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_TUPLE:
        s = _tuple_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_UNIT:
        s = _unit_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_VECTOR:
        s = _vector_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_VOID:
        s = _void_to_string(type, obj, seen, excpt, ctx);
        break;

     case BINPAC_TYPE_NONE:
        fprintf(stderr, "internal error: BinPAC type not set in HILTI rtti object when rendering (HILTI type: %d/%s)\n", type->type, type->tag);
        abort();

     default:
        fprintf(stderr, "internal error: BinPAC type %" PRIu64 " not supported fo rendering (HILTI type: %d/%s)\n", id, type->type, type->tag);
        abort();
    }

    if ( type->gc )
        __hlt_pointer_stack_pop_back(seen);

    return s;
}

hlt_string binpac_object_to_string(const hlt_type_info* type, void* obj, hlt_exception** excpt, hlt_execution_context* ctx)
{
    __hlt_pointer_stack seen;
    __hlt_pointer_stack_init(&seen);

    hlt_string s = _object_to_string(type, obj, &seen, excpt, ctx);

    __hlt_pointer_stack_destroy(&seen);

    return s;
}

void binpachilti_print(const hlt_type_info* type, void* obj, int8_t newline, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = binpac_object_to_string(type, obj, excpt, ctx);

    if ( hlt_check_exception(excpt) )
        return;

    int old_state;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);

    flockfile(stdout);

    hlt_string_print(stdout, s, 0, excpt, ctx);

    if ( newline )
        fputc('\n', stdout);

    fflush(stdout);

unlock:
    funlockfile(stdout);
    pthread_setcancelstate(old_state, NULL);
}
