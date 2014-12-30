// A proof-of-concept Wireshark dissector leveraging BinPAC++.
//
//  - Add more data types
//
//  - Release dynamic memory
//  - Error handling pretty much completely missing.

#include <ctype.h>

#include "./config.h"
#include <epan/packet.h>
#include <epan/prefs.h>

#include "binpacshark.h"

static const int max_fields = 100;
static const int max_dissectors = 5;

struct dissector {
    int proto;
    gint ett;
    gint ett_sub;
    int fields[max_fields];
    int num_fields;
    int display_field;
    const char* name;
    const char* description;
    binpac_parser* parser;
};

struct dissector dissectors[max_dissectors];
int num_dissectors = 0;

void display_field(struct dissector* d, proto_tree *tree, const char* name, const hlt_type_info* type_info, void* obj, int64_t begin, int64_t end, tvbuff_t *tvb, hlt_exception** excpt, hlt_execution_context* ctx);
void register_field(struct dissector* d, int parent, const char* path, const char* name, const hlt_type_info* type_info, hlt_exception** excpt, hlt_execution_context* ctx);

void display_unit(struct dissector* d, proto_tree *tree, const hlt_type_info* type, void* obj, tvbuff_t *tvb, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_unit_cookie cookie = 0;
    binpac_unit_item item;

    void* unit = *(void**)obj;

    while ( (cookie = binpac_unit_iterate(&item, type, unit, 0, cookie, excpt, ctx)) ) {
        if ( ! item.name )
            // Don't include fields that have no name.
            continue;

        display_field(d, tree, item.name, item.type, item.value, item.begin, item.end, tvb, excpt, ctx);
    }
}

void display_field(struct dissector* d, proto_tree *tree, const char* name, const hlt_type_info* type, void* obj, int64_t begin, int64_t end, tvbuff_t *tvb, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_type_id id = binpac_type_get_id(type, excpt, ctx);

    int field = d->fields[d->display_field++];

    gint fstart = begin;
    gint flength = (end - begin);

    if ( ! obj ) {
        proto_tree_add_string(tree, field, tvb, fstart, flength, "(not set)");
        return;
    }

    switch ( id ) {
     case BINPAC_TYPE_BYTES: {
         hlt_bytes* b = *(hlt_bytes**)obj;
         hlt_bytes_size n = hlt_bytes_len(b, excpt, ctx);
         int8_t* dst = hlt_malloc(n);
         hlt_bytes_to_raw(dst, n, b, excpt, ctx);
         proto_tree_add_string(tree, field, tvb, fstart, n, (const char *)dst);
         break;
     }

     case BINPAC_TYPE_UNIT: {
         //proto_item* ti = proto_tree_add_item(tree, d->proto, tvb, 0, -1, ENC_NA);
         proto_tree *subtree = proto_tree_add_subtree(tree, tvb, fstart, flength, d->ett_sub, NULL, name);
         display_unit(d, subtree, type, obj, tvb, excpt, ctx);
         break;
     }

     case BINPAC_TYPE_INTEGER_SIGNED: {
         int64_t i = hlt_int_to_int64(type, obj, 0, excpt, ctx);
         proto_tree_add_int64(tree, field, tvb, fstart, flength, i);
         break;
     }

     case BINPAC_TYPE_INTEGER_UNSIGNED: {
         int64_t i = hlt_int_to_int64(type, obj, HLT_CONVERT_UNSIGNED, excpt, ctx);
         proto_tree_add_uint64(tree, field, tvb, fstart, flength, i);
         break;
     }

     case BINPAC_TYPE_ADDRESS:
     case BINPAC_TYPE_BOOL:
     case BINPAC_TYPE_BITFIELD:
     case BINPAC_TYPE_DOUBLE:
     case BINPAC_TYPE_EMBEDDED_OBJECT:
     case BINPAC_TYPE_ENUM:

     case BINPAC_TYPE_INTERVAL:
     case BINPAC_TYPE_ITERATOR_BYTES:
     case BINPAC_TYPE_LIST:
     case BINPAC_TYPE_MAP:
     case BINPAC_TYPE_OPTIONAL:
     case BINPAC_TYPE_PORT:
     case BINPAC_TYPE_REGEXP:
     case BINPAC_TYPE_SET:
     case BINPAC_TYPE_SINK:
     case BINPAC_TYPE_STRING:
     case BINPAC_TYPE_TIME:
     case BINPAC_TYPE_TUPLE:
     case BINPAC_TYPE_VECTOR:
     case BINPAC_TYPE_VOID:
        proto_tree_add_string(tree, field, tvb, fstart, flength, "<Field type unsupported>");
        break;

     case BINPAC_TYPE_NONE:
        fprintf(stderr, "internal error: BinPAC type not set in HILTI rtti object when rendering (HILTI type: %d/%s)\n", type->type, type->tag);
        abort();

     default:
        fprintf(stderr, "internal error: BinPAC type %" PRIu64 " not supported fo rendering (HILTI type: %d/%s)\n", id, type->type, type->tag);
        abort();
    }
}

void register_unit(struct dissector* d, int parent, const char* path, const hlt_type_info* type,  hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_unit_cookie cookie = 0;
    binpac_unit_item item;

    while ( (cookie = binpac_unit_iterate(&item, type, 0, 0, cookie, excpt, ctx)) ) {
        if ( ! item.name )
            // Don't include fields that have no name.
            continue;

        register_field(d, parent, path, item.name, item.type, excpt, ctx);
    }
}

void register_field(struct dissector* d, int parent, const char* path, const char* name, const hlt_type_info* type, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_type_id id = binpac_type_get_id(type, excpt, ctx);

    hf_register_info* hfi = (hf_register_info*) hlt_malloc(sizeof(header_field_info));
    int* field = &d->fields[d->num_fields++];

    char* fname = malloc(128);
    snprintf(fname, 128, "%s.%s", path, name);

    switch ( id ) {
     case BINPAC_TYPE_BYTES:  {
        hf_register_info tmp = { field, { name, fname, FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL } };
        *hfi = tmp;
        proto_register_field_array(parent, hfi, 1);
        break;
     }

     case BINPAC_TYPE_UNIT: {
         register_unit(d, parent, fname, type, excpt, ctx);
         break;
     }

     case BINPAC_TYPE_INTEGER_SIGNED: {
        hf_register_info tmp = { field, { name, fname, FT_INT64, BASE_DEC, NULL, 0x0, NULL, HFILL } };
        *hfi = tmp;
        proto_register_field_array(parent, hfi, 1);
        break;
     }

     case BINPAC_TYPE_INTEGER_UNSIGNED: {
        hf_register_info tmp = { field, { name, fname, FT_UINT64, BASE_DEC, NULL, 0x0, NULL, HFILL } };
        *hfi = tmp;
        proto_register_field_array(parent, hfi, 1);
        break;
     }

     case BINPAC_TYPE_ADDRESS:
     case BINPAC_TYPE_BOOL:
     case BINPAC_TYPE_BITFIELD:
     case BINPAC_TYPE_DOUBLE:
     case BINPAC_TYPE_EMBEDDED_OBJECT:
     case BINPAC_TYPE_ENUM:
     case BINPAC_TYPE_INTERVAL:
     case BINPAC_TYPE_ITERATOR_BYTES:
     case BINPAC_TYPE_LIST:
     case BINPAC_TYPE_MAP:
     case BINPAC_TYPE_OPTIONAL:
     case BINPAC_TYPE_PORT:
     case BINPAC_TYPE_REGEXP:
     case BINPAC_TYPE_SET:
     case BINPAC_TYPE_SINK:
     case BINPAC_TYPE_STRING:
     case BINPAC_TYPE_TIME:
     case BINPAC_TYPE_TUPLE:
     case BINPAC_TYPE_VECTOR:
     case BINPAC_TYPE_VOID: {
        hf_register_info tmp = { field, { name, fname, FT_STRINGZ, BASE_NONE, NULL, 0x0, "<Field type unsupported>", HFILL } };
        *hfi = tmp;
        proto_register_field_array(parent, hfi, 1);
        break;
     }

     case BINPAC_TYPE_NONE:
        fprintf(stderr, "internal error: BinPAC type not set in HILTI rtti object when rendering (HILTI type: %d/%s)\n", type->type, type->tag);
        abort();

     default:
        fprintf(stderr, "internal error: BinPAC type %" PRIu64 " not supported fo rendering (HILTI type: %d/%s)\n", id, type->type, type->tag);
        abort();
    }
}

// Dissect fields.

int dissect_one(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data)
{
    int i = 0;

    for ( i = 0; i < num_dissectors; i++ ) {
        if ( strcmp(dissectors[i].name, pinfo->current_proto) == 0 )
            break;
    }

    if ( i >= num_dissectors ) {
        fprintf(stderr, "internal binpac error: current proto is not ours (%s)\n", pinfo->current_proto);
        return 0;
    }

    struct dissector* d = &dissectors[i];

    col_set_str(pinfo->cinfo, COL_PROTOCOL, d->description);
    col_set_str(pinfo->cinfo, COL_INFO, d->description);

    if ( ! tree )
        return 0;

    proto_item* ti = proto_tree_add_item(tree, d->proto, tvb, 0, -1, ENC_NA);
    proto_tree *our_tree = proto_item_add_subtree(ti, d->ett);

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    guint wlen = tvb_length_remaining(tvb, 0);
    guint8* wdata = (guint8 *)tvb_memdup(NULL, tvb, 0, wlen);
    assert(wdata);

    hlt_bytes* input = hlt_bytes_new_from_data((int8_t*)wdata, wlen, &excpt, ctx);
    hlt_bytes_freeze(input, 1, &excpt, ctx);

    void *pobj = (*d->parser->parse_func)(input, 0, &excpt, ctx);

    d->display_field = 0;
    display_unit(d, our_tree, d->parser->type_info, &pobj, tvb, &excpt, ctx);

//    GC_DTOR_GENERIC(&pobj, d->parser->type_info, ctx);
//    GC_DTOR(input, hlt_bytes, ctx);
//    wmem_free(0, wdata);

    return 0;
}

void proto_register_one(binpac_parser* p, int i)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    char* name = hlt_string_to_native(p->name, &excpt, ctx);
    char* fname = strdup(name);

    char* description = hlt_string_to_native(p->description, &excpt, ctx);

    for ( char* p = fname; *p; p++ ) {
        if ( *p == ':' ) // WS doesn't like these.
            *p = '_';
        *p = tolower(*p);
    }

    fprintf(stderr, "%s - %s\n", name, description);

    struct dissector* d = &dissectors[i];
    d->proto = proto_register_protocol (name, name, fname);
    d->ett = -1;
    d->ett_sub = -1;
    d->num_fields = 0;
    d->display_field = 0;
    d->name = name;
    d->description = description;
    d->parser = p;

    static gint *ett[2];
    ett[0] = &d->ett;
    ett[1] = &d->ett_sub;
    proto_register_subtree_array(ett, 2);

    register_unit(d, d->proto, fname, p->type_info, &excpt, ctx);
}

void proto_register_one_handoff(binpac_parser* p, int i)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    dissector_handle_t handle = new_create_dissector_handle(dissect_one, dissectors[i].proto);

    hlt_iterator_list j = hlt_list_begin(p->ports, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(p->ports, &excpt, ctx);

    while ( ! (hlt_iterator_list_eq(j, end, &excpt, ctx) || excpt) ) {
        hlt_port p = *(hlt_port*) hlt_iterator_list_deref(j, &excpt, ctx);
        const char* tag = (p.proto == HLT_PORT_UDP) ? "udp.port" : "tcp.port";
        dissector_add_uint(tag, p.port, handle);
        j = hlt_iterator_list_incr(j, &excpt, ctx);
    }

    // FIXME: Remove.
    dissector_add_uint("tcp.port", 12345, handle);
}

void proto_register_all_binpac()
{
    fprintf(stderr, "Initializing HILTI/BinPAC++ runtime\n");

    pac2_init();

    fprintf(stderr, "Registering all binpac parsers\n");

    hlt_list* parsers = pac2_get_parsers();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);
        proto_register_one(p, num_dissectors++);
        i = hlt_iterator_list_incr(i, &excpt, ctx);
    }
}

void proto_register_all_binpac_handoff(void)
{
    fprintf(stderr, "Registering handoffs\n");

    hlt_list* parsers = pac2_get_parsers();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    int j = 0;

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);
        proto_register_one_handoff(p, j++);
        i = hlt_iterator_list_incr(i, &excpt, ctx);
    }
}
