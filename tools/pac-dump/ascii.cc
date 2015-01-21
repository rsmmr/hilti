
extern "C" {
    #include <libbinpac++.h>
}

static void newline(int indent)
{
    fputs("\n", stdout);
    while ( indent-- )
    fputs("  ", stdout);
}

static void newline_and_indent(int& indent)
{
    ++indent;
    newline(indent);
}

static void newline_and_dedent(int& indent)
{
    --indent;
    newline(indent);
}


extern void ascii_dump_object(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx);
static void ascii_dump_tuple(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx);

static void ascii_dump_addr(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_bitfield(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    ascii_dump_tuple(type, obj, indent, excpt, ctx);
}

static void ascii_dump_bool(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_bytes(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_double(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_embedded(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    fputs("object(", stdout);
    hilti_print(type, obj, 0, excpt, ctx);
    fputs(")", stdout);
}

static void ascii_dump_enum(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_uint(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_sint(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_interval(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_iter_bytes(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_list(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_list* list = *(hlt_list **)obj;
    const hlt_type_info* etype = hlt_list_element_type(type, excpt, ctx);

    hlt_iterator_list i = hlt_list_begin(list, excpt, ctx);
    hlt_iterator_list end = hlt_list_end(list, excpt, ctx);

    fputs("[", stdout);
    newline_and_indent(indent);

    int first = 1;

    while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {
        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        void* ep = hlt_iterator_list_deref(i, excpt, ctx);
        ascii_dump_object(etype, ep, indent, excpt, ctx);

        i = hlt_iterator_list_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("]", stdout);
}

static void ascii_dump_map(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_map* map = *(hlt_map **)obj;
    const hlt_type_info* ktype = hlt_map_key_type(type, excpt, ctx);
    const hlt_type_info* vtype = hlt_map_value_type(type, excpt, ctx);

    hlt_iterator_map i = hlt_map_begin(map, excpt, ctx);
    hlt_iterator_map end = hlt_map_end(excpt, ctx);

    fputs("{", stdout);
    newline_and_indent(indent);

    int first = 1;

    while ( ! hlt_iterator_map_eq(i, end, excpt, ctx) ) {
        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        void* kp = hlt_iterator_map_deref_key(i, excpt, ctx);
        void* vp = hlt_iterator_map_deref_value(i, excpt, ctx);
        ascii_dump_object(ktype, kp, indent, excpt, ctx);
        fputs(": ", stdout);
        ascii_dump_object(vtype, vp, indent, excpt, ctx);

        i = hlt_iterator_map_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("}", stdout);
}

static void ascii_dump_optional(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_union* u = (hlt_union *)obj;

    void* v = hlt_union_get(u, -1, excpt, ctx);

    if ( ! v )
        fputs("\"(not set)\"", stdout);

    hlt_union_field f = hlt_union_get_type(type, u, -1, excpt, ctx);
    ascii_dump_object(f.type, v, indent, excpt, ctx);
}

static void ascii_dump_port(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_regexp(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_set(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* set = *(hlt_set **)obj;
    const hlt_type_info* etype = hlt_set_element_type(type, excpt, ctx);

    hlt_iterator_set i = hlt_set_begin(set, excpt, ctx);
    hlt_iterator_set end = hlt_set_end(excpt, ctx);

    fputs("}", stdout);
    newline_and_indent(indent);

    int first = 1;

    while ( ! hlt_iterator_set_eq(i, end, excpt, ctx) ) {
        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        void* ep = hlt_iterator_set_deref(i, excpt, ctx);
        ascii_dump_object(etype, ep, indent, excpt, ctx);

        i = hlt_iterator_set_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("}", stdout);
}

static void ascii_dump_sink(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    fputs("<sink>", stdout);
}

static void ascii_dump_string(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_string s = *(hlt_string *)obj;
    hlt_string_print(stdout, s, 0, excpt, ctx);
}

static void ascii_dump_time(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void ascii_dump_tuple(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* tuple = obj;

    fputs("(", stdout);
    newline_and_indent(indent);

    int first = 0;

    for ( int i = 0; i < hlt_tuple_length(type, excpt, ctx); i++ ) {
        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        void* ev = hlt_tuple_get(type, tuple, i, excpt, ctx);
        hlt_tuple_element et = hlt_tuple_get_type(type, i, excpt, ctx);

        if ( et.name && *et.name ) {
            fputs(et.name, stdout);
            fputs("=", stdout);
        }

        ascii_dump_object(et.type, ev, indent, excpt, ctx);
    }

    newline_and_dedent(indent);
    fputs(")", stdout);
}

static void ascii_dump_unit(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* unit = *(void**)obj;

    if ( ! unit ) {
        fputs("(Null)", stdout);
        return;
    }

    binpac_unit_cookie cookie = 0;
    binpac_unit_item item;

    int first = 1;

    fputs("<", stdout);
    newline_and_indent(indent);

    while ( (cookie = binpac_unit_iterate(&item, type, unit, 0, cookie, excpt, ctx)) ) {

        if ( ! item.value )
            // Don't print fields that aren't set.
            continue;

        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        if ( item.name )
            fputs(item.name, stdout);
        else
            fputs("<\?\?\?>", stdout);

        fputs("=", stdout);

        if ( item.value )
            ascii_dump_object(item.type, item.value, indent, excpt, ctx);
        else
            fputs("(not set)", stdout);

        first = 0;
    }

    newline_and_dedent(indent);
    fputs(">", stdout);
}

static void ascii_dump_vector(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_vector* vector = *(hlt_vector **)obj;
    const hlt_type_info* etype = hlt_vector_element_type(type, excpt, ctx);

    hlt_iterator_vector i = hlt_vector_begin(vector, excpt, ctx);
    hlt_iterator_vector end = hlt_vector_end(vector, excpt, ctx);

    fputs("[", stdout);
    newline_and_indent(indent);

    int first = 1;

    while ( ! hlt_iterator_vector_eq(i, end, excpt, ctx) ) {
        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        void* ep = hlt_iterator_vector_deref(i, excpt, ctx);
        ascii_dump_object(etype, ep, indent, excpt, ctx);

        i = hlt_iterator_vector_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("]", stdout);
}

static void ascii_dump_void(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

void ascii_dump_object(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_type_id id = binpac_type_get_id(type, excpt, ctx);

    switch ( id ) {
     case BINPAC_TYPE_ADDRESS:
        ascii_dump_addr(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_BOOL:
        ascii_dump_bool(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_BITFIELD:
        ascii_dump_bitfield(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_BYTES:
        ascii_dump_bytes(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_DOUBLE:
        ascii_dump_double(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_EMBEDDED_OBJECT:
        ascii_dump_embedded(type, obj, indent, excpt, ctx);
        break
        ;
     case BINPAC_TYPE_ENUM:
        ascii_dump_enum(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_INTEGER_SIGNED:
        ascii_dump_sint(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_INTEGER_UNSIGNED:
        ascii_dump_uint(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_INTERVAL:
        ascii_dump_interval(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_ITERATOR_BYTES:
        ascii_dump_iter_bytes(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_LIST:
        ascii_dump_list(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_MAP:
        ascii_dump_map(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_OPTIONAL:
        ascii_dump_optional(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_PORT:
        ascii_dump_port(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_REGEXP:
        ascii_dump_regexp(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_SET:
        ascii_dump_set(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_SINK:
        ascii_dump_sink(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_STRING:
        ascii_dump_string(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_TIME:
        ascii_dump_time(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_TUPLE:
        ascii_dump_tuple(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_UNIT:
        ascii_dump_unit(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_VECTOR:
        ascii_dump_vector(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_VOID:
        ascii_dump_void(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_NONE:
        fprintf(stderr, "internal error: BinPAC type not set in HILTI rtti object when rendering (HILTI type: %d/%s)\n", type->type, type->tag);
        abort();

     default:
        fprintf(stderr, "internal error: BinPAC type %" PRIu64 " not supported fo rendering (HILTI type: %d/%s)\n", id, type->type, type->tag);
        abort();
    }
}
