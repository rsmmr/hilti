
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


extern void json_dump_object(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx);
static void json_dump_tuple(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx);

static void hilti_print_as_string(const hlt_type_info* type, void* obj, int newline, hlt_exception** excpt, hlt_execution_context* ctx)
{
    fputc('"', stdout);

    hlt_string s = hlt_object_to_string(type, obj, 0, excpt, ctx);

    if ( ! s )
        // Empty string.
        return;

    hlt_string_size len = (s->len);

    int32_t cp;
    const int8_t* p = s->bytes;
    const int8_t* e = p + len;

    while ( p < e ) {
        ssize_t n = utf8proc_iterate((const uint8_t *)p, e - p, &cp);

        if ( n < 0 ) {
            hlt_set_exception(excpt, &hlt_exception_value_error, 0, ctx);
            return;
        }

        switch ( cp ) {
         case '"':
            fputs("\\\"", stdout);
            break;
         case '\\':
            fputs("\\\\", stdout);
            break;
         case '/':
            fputs("\\/", stdout);
            break;
         case '\b':
            fputs("\\b", stdout);
            break;
         case '\f':
            fputs("\\f", stdout);
            break;
         case '\n':
            fputs("\\n", stdout);
            break;
         case '\r':
            fputs("\\r", stdout);
            break;
         case '\t':
            fputs("\\t", stdout);
            break;

         default: {
             if ( cp < 128 )
                 fputc(cp, stdout);
             else {
                 if ( cp < (1 << 16) )
                     fprintf(stdout, "\\u%04x", cp);
                 else
                fprintf(stdout, "\\U%08x", cp);
             }
         }
        }

        p += n;
    }

    fputc('"', stdout);
}

static void json_dump_addr(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

static void json_dump_bitfield(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    json_dump_tuple(type, obj, indent, excpt, ctx);
}

static void json_dump_bool(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int8_t b = *((int8_t*)obj);
    fputs(b ? "true" : "false", stdout);
}

static void json_dump_bytes(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

static void json_dump_double(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void json_dump_embedded(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    fputs("\"object(...)\")", stdout);
}

static void json_dump_enum(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

static void json_dump_uint(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void json_dump_sint(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print(type, obj, 0, excpt, ctx);
}

static void json_dump_interval(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    double d = hlt_interval_to_double(type, obj, 0, excpt, ctx);
    fprintf(stdout, "%.6f", d);
}

static void json_dump_iter_bytes(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

static void json_dump_list(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
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
        json_dump_object(etype, ep, indent, excpt, ctx);

        i = hlt_iterator_list_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("]", stdout);
}

static void json_dump_map(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
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
        json_dump_object(ktype, kp, indent, excpt, ctx);
        fputs(": ", stdout);
        json_dump_object(vtype, vp, indent, excpt, ctx);

        i = hlt_iterator_map_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("}", stdout);
}

static void json_dump_optional(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_union* u = (hlt_union *)obj;

    void* v = hlt_union_get(u, -1, excpt, ctx);

    if ( ! v )
        fputs("\"(not set)\"", stdout);

    hlt_union_field f = hlt_union_get_type(type, u, -1, excpt, ctx);
    json_dump_object(f.type, v, indent, excpt, ctx);
}

static void json_dump_port(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    int64_t p = hlt_port_to_int64(type, obj, 0, excpt, ctx);
    fprintf(stdout, "%" PRId64, p);
}

static void json_dump_regexp(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

static void json_dump_set(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_set* set = *(hlt_set **)obj;
    const hlt_type_info* etype = hlt_set_element_type(type, excpt, ctx);

    hlt_iterator_set i = hlt_set_begin(set, excpt, ctx);
    hlt_iterator_set end = hlt_set_end(excpt, ctx);

    fputs("[", stdout);
    newline_and_indent(indent);

    int first = 1;

    while ( ! hlt_iterator_set_eq(i, end, excpt, ctx) ) {
        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        void* ep = hlt_iterator_set_deref(i, excpt, ctx);
        json_dump_object(etype, ep, indent, excpt, ctx);

        i = hlt_iterator_set_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("]", stdout);
}

static void json_dump_sink(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    fputs("\"<sink>\"", stdout);
}

static void json_dump_string(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

static void json_dump_time(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    double d = hlt_time_to_double(type, obj, 0, excpt, ctx);
    fprintf(stdout, "%.6f", d);
}

static void json_dump_tuple(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* tuple = obj;

    fputs("[", stdout);
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
            fputs(": ", stdout);
        }

        json_dump_object(et.type, ev, indent, excpt, ctx);
    }

    newline_and_dedent(indent);
    fputs("]", stdout);
}

static void json_dump_unit(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    void* unit = *(void**)obj;

    if ( ! unit ) {
        fputs("(Null)", stdout);
        return;
    }

    binpac_unit_cookie cookie = 0;
    binpac_unit_item item;

    int first = 1;

    fputs("{", stdout);
    newline_and_indent(indent);

    while ( (cookie = binpac_unit_iterate(&item, type, unit, 0, cookie, excpt, ctx)) ) {

        if ( ! item.value )
            // Don't print fields that aren't set.
            continue;

        if ( item.hide )
            // Not to be shown.
            continue;

        if ( ! first ) {
            fputs(",", stdout);
            newline(indent);
        }

        if ( item.name )
            fputs(item.name, stdout);
        else
            fputs("<\?\?\?>", stdout);

        fputs(": ", stdout);

        if ( item.value )
            json_dump_object(item.type, item.value, indent, excpt, ctx);
        else
            fputs("(not set)", stdout);

        first = 0;
    }

    newline_and_dedent(indent);
    fputs("}", stdout);
}

static void json_dump_vector(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
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
        json_dump_object(etype, ep, indent, excpt, ctx);

        i = hlt_iterator_vector_incr(i, excpt, ctx);
        first = 0;
    }

    newline_and_dedent(indent);
    fputs("]", stdout);
}

static void json_dump_void(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hilti_print_as_string(type, obj, 0, excpt, ctx);
}

void json_dump_object(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_type_id id = binpac_type_get_id(type, excpt, ctx);

    switch ( id ) {
     case BINPAC_TYPE_ADDRESS:
        json_dump_addr(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_BOOL:
        json_dump_bool(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_BITFIELD:
        json_dump_bitfield(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_BYTES:
        json_dump_bytes(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_DOUBLE:
        json_dump_double(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_EMBEDDED_OBJECT:
        json_dump_embedded(type, obj, indent, excpt, ctx);
        break
        ;
     case BINPAC_TYPE_ENUM:
        json_dump_enum(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_INTEGER_SIGNED:
        json_dump_sint(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_INTEGER_UNSIGNED:
        json_dump_uint(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_INTERVAL:
        json_dump_interval(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_ITERATOR_BYTES:
        json_dump_iter_bytes(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_LIST:
        json_dump_list(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_MAP:
        json_dump_map(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_OPTIONAL:
        json_dump_optional(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_PORT:
        json_dump_port(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_REGEXP:
        json_dump_regexp(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_SET:
        json_dump_set(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_SINK:
        json_dump_sink(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_STRING:
        json_dump_string(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_TIME:
        json_dump_time(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_TUPLE:
        json_dump_tuple(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_UNIT:
        json_dump_unit(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_VECTOR:
        json_dump_vector(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_VOID:
        json_dump_void(type, obj, indent, excpt, ctx);
        break;

     case BINPAC_TYPE_NONE:
        fprintf(stderr, "internal error: BinPAC type not set in HILTI rtti object when rendering (HILTI type: %d/%s)\n", type->type, type->tag);
        abort();

     default:
        fprintf(stderr, "internal error: BinPAC type %" PRIu64 " not supported fo rendering (HILTI type: %d/%s)\n", id, type->type, type->tag);
        abort();
    }
}
