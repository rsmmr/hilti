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
#include <epan/conversation.h>

#include "binpacshark.h"

extern hlt_string binpac_object_to_string(const hlt_type_info* type, void* obj, hlt_exception** excpt, hlt_execution_context* ctx);

static const int max_fields = 100;
static const int max_dissectors = 50;

struct dissector {
    int proto;
    gint ett;
    gint ett_sub;
    gint *ett_array[2];
    int fields[max_fields];
    int num_fields;
    const char* name;
    const char* description;
    binpac_parser* parser;
};

enum Field {
    FIELD_STRING,
    FIELD_INT64,
    FIELD_UINT64,
    FIELD_BOOL,
    FIELD_BYTES,
    MAX_FIELDS,
};

int hf_fields[MAX_FIELDS];

static hf_register_info hf[] = {
        { &hf_fields[FIELD_STRING],
          { "String",    "binpac.string",
            FT_STRING,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_fields[FIELD_INT64],
          { "int<64>",    "binpac.int64",
            FT_INT64,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_fields[FIELD_UINT64],
          { "uint<64>",    "binpac.uint64",
            FT_UINT64,  BASE_DEC, NULL, 0x0,
            NULL, HFILL }},

        { &hf_fields[FIELD_BOOL],
          { "bool",    "binpac.bool",
            FT_BOOLEAN,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }},

        { &hf_fields[FIELD_BYTES],
          { "bytes",    "binpac.bytes",
            FT_STRING,  BASE_NONE, NULL, 0x0,
            NULL, HFILL }}
    };

struct dissector dissectors[max_dissectors];
int num_dissectors = 0;

void display_field(struct dissector* d, proto_tree *tree, const char* name, const hlt_type_info* type_info, void* obj, int64_t begin, int64_t end, tvbuff_t *tvb, hlt_exception** excpt, hlt_execution_context* ctx);

void display_unit(struct dissector* d, proto_tree *tree, const hlt_type_info* type, void* obj, tvbuff_t *tvb, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_unit_cookie cookie = 0;
    binpac_unit_item item;

    void* unit = *(void**)obj;

    while ( (cookie = binpac_unit_iterate(&item, type, unit, 0, cookie, excpt, ctx)) ) {
        if ( ! item.name )
            // Don't include fields that have no name.
            continue;

        if ( item.hide )
            // A no show.
            continue;

        display_field(d, tree, item.name, item.type, item.value, item.begin, item.end, tvb, excpt, ctx);
    }
}

void display_field(struct dissector* d, proto_tree *tree, const char* name, const hlt_type_info* type, void* obj, int64_t begin, int64_t end, tvbuff_t *tvb, hlt_exception** excpt, hlt_execution_context* ctx)
{
    binpac_type_id id = binpac_type_get_id(type, excpt, ctx);

    gint fstart = begin;
    gint flength = (end - begin);

    if ( ! obj ) {
        // proto_tree_add_string(tree, hf_fields[FIELD_STRING], tvb, fstart, flength, "(not set)");
        return;
    }

    switch ( id ) {
     case BINPAC_TYPE_STRING:
     case BINPAC_TYPE_TIME:
     case BINPAC_TYPE_TUPLE:
     case BINPAC_TYPE_INTERVAL:
     case BINPAC_TYPE_ITERATOR_BYTES:
     case BINPAC_TYPE_OPTIONAL:
     case BINPAC_TYPE_PORT:
     case BINPAC_TYPE_REGEXP:
     case BINPAC_TYPE_ADDRESS:
     case BINPAC_TYPE_BOOL:
     case BINPAC_TYPE_DOUBLE:
     case BINPAC_TYPE_ENUM:
     case BINPAC_TYPE_INTEGER_UNSIGNED:
     case BINPAC_TYPE_INTEGER_SIGNED:
     case BINPAC_TYPE_BYTES: {
         hlt_string s = binpac_object_to_string(type, obj, excpt, ctx);
         char* ns = hlt_string_to_native(s, excpt, ctx);
         proto_tree_add_text(tree, tvb, fstart, flength, "%s: %s", name, ns);
         break;
     }

     case BINPAC_TYPE_UNIT: {
         proto_tree *subtree = proto_tree_add_subtree(tree, tvb, fstart, flength, d->ett_sub, NULL, name);
         display_unit(d, subtree, type, obj, tvb, excpt, ctx);
         break;
     }

     case BINPAC_TYPE_LIST: {
         hlt_list* list = *(hlt_list **)obj;
         const hlt_type_info* etype = hlt_list_element_type(type, excpt, ctx);

         hlt_iterator_list i = hlt_list_begin(list, excpt, ctx);
         hlt_iterator_list end = hlt_list_end(list, excpt, ctx);

         while ( ! hlt_iterator_list_eq(i, end, excpt, ctx) ) {
             void* ep = hlt_iterator_list_deref(i, excpt, ctx);

             display_field(d, tree, name, etype, ep, 0, 0, tvb, excpt, ctx);

             i = hlt_iterator_list_incr(i, excpt, ctx);
         }
     }

     case BINPAC_TYPE_VOID:
     case BINPAC_TYPE_SINK:
         // Ignore.
         break;

     case BINPAC_TYPE_BITFIELD:
     case BINPAC_TYPE_EMBEDDED_OBJECT:
     case BINPAC_TYPE_MAP:
     case BINPAC_TYPE_SET:
     case BINPAC_TYPE_VECTOR:
        proto_tree_add_string(tree, hf_fields[FIELD_STRING], tvb, fstart, flength, "<Field type unsupported>");
        break;

     case BINPAC_TYPE_NONE:
        fprintf(stderr, "internal error: BinPAC type not set in HILTI rtti object when rendering (HILTI type: %d/%s)\n", type->type, type->tag);
        abort();

     default:
        fprintf(stderr, "internal error: BinPAC type %" PRIu64 " not supported fo rendering (HILTI type: %d/%s)\n", id, type->type, type->tag);
        abort();
    }
}

// Dissect fields.

struct endpoint {
    hlt_bytes* input;
    hlt_exception* resume;
    address addr;
};

struct conv_data {
    struct endpoint orig;
    struct endpoint resp;
};

int dissect_one(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *p)
{
    fprintf(stderr, "Entering dissect one: length=%u\n", tvb_reported_length(tvb));

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

    col_set_str(pinfo->cinfo, COL_PROTOCOL, d->name);
    col_set_str(pinfo->cinfo, COL_INFO, d->description);

    if ( ! tree )
        return 0;

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    conversation_t *conversation = find_or_create_conversation(pinfo);
    struct conv_data* cd = (struct conv_data*) conversation_get_proto_data(conversation, d->proto);

    if ( ! cd ) {
        cd = (struct conv_data*)hlt_malloc(sizeof(struct conv_data));
        cd->orig.input = hlt_bytes_new(&excpt, ctx);;
        cd->resp.input = hlt_bytes_new(&excpt, ctx);;
        GC_CCTOR(cd->orig.input, hlt_bytes, ctx);
        GC_CCTOR(cd->resp.input, hlt_bytes, ctx);
        cd->orig.resume = 0;
        cd->resp.resume = 0;
        copy_address(&cd->orig.addr, &pinfo->src);
        copy_address(&cd->resp.addr, &pinfo->dst);
        conversation_add_proto_data(conversation, d->proto, cd);
    }

    struct endpoint* ep = 0;

    if ( addresses_equal(&pinfo->src, &cd->orig.addr) )
        ep = &cd->orig;
    else if ( addresses_equal(&pinfo->src, &cd->resp.addr) )
        ep = &cd->resp;
    else {
        fprintf(stderr, "unknown address!\n");
        exit(1);
    }

    if ( ! ep->input ) {
        fprintf(stderr, "ignoring further input\n");
        return 0;
    }

    guint offset = 0;

    void* pobj = 0;

    while( offset < tvb_reported_length(tvb) ) {
        gint available = tvb_reported_length_remaining(tvb, offset);
        fprintf(stderr, "  Next chunk in dissect one:available=%d\n", available);

        if ( available < 0 ) {
            /* we ran out of data: ask for more */
            pinfo->desegment_offset = offset;
            pinfo->desegment_len = DESEGMENT_ONE_MORE_SEGMENT;
            fprintf(stderr, "Leaving dissect one, asking for more\n");
            return (offset + available);
        }

        int8_t buffer[available];
        tvb_memcpy(tvb, buffer, offset, available);

        hlt_bytes* next = hlt_bytes_new_from_data_copy(buffer, available, &excpt, ctx);
        hlt_bytes_append(ep->input, next, &excpt, ctx);

        hlt_string s = hlt_object_to_string(&hlt_type_info_hlt_bytes, &ep->input, 0, &excpt, ctx);
        hlt_string_print(stderr, s, 1, &excpt, ctx);

        if ( pinfo->ptype != PT_TCP ) // TODO: How to determine end for TCP?
            hlt_bytes_freeze(ep->input, 1, &excpt, ctx);

        excpt = 0;

        if ( ! ep->resume ) {
            pobj = (*d->parser->parse_func)(ep->input, 0, &excpt, ctx);
            GC_CCTOR_GENERIC(&pobj, d->parser->type_info, ctx);
        }

        else {
            pobj = (*d->parser->resume_func)(ep->resume, &excpt, ctx);
            GC_CCTOR_GENERIC(&pobj, d->parser->type_info, ctx);
        }

        if ( excpt ) {
            if ( hlt_exception_is_yield(excpt) ) {
                ep->resume = excpt;
                excpt = 0;
                GC_CCTOR(ep->resume, hlt_exception, ctx);
            }

            else {
                fprintf(stderr, "Exception!\n");
                hlt_exception_print(excpt, ctx);
                GC_CLEAR(ep->input, hlt_bytes, ctx);
                break;
            }
        }

        else {
            fprintf(stderr, "all done!\n");
            conversation_delete_proto_data(conversation, d->proto);
        }

        offset += (guint)available;
    }


    if ( pobj ) {
        fprintf(stderr, "Displaying unit\n");
        proto_item* ti = proto_tree_add_item(tree, d->proto, tvb, 0, -1, ENC_NA);
        proto_tree *our_tree = proto_item_add_subtree(ti, d->ett);
        display_unit(d, our_tree, d->parser->type_info, &pobj, tvb, &excpt, ctx);
    }

    fprintf(stderr, "Leaving dissect one\n");

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

    fprintf(stderr, "Register one: %s - %s\n", name, description);

    struct dissector* d = &dissectors[i];

    d->proto = proto_register_protocol (name, name, fname);
    d->ett = -1;
    d->ett_sub = -1;
    d->ett_array[0] = &d->ett;
    d->ett_array[1] = &d->ett_sub;
    d->num_fields = 0;
    d->name = name;
    d->description = description;
    d->parser = p;
    GC_CCTOR(d->parser, hlt_BinPACHilti_Parser, ctx);

    prefs_register_protocol(d->proto, NULL);

    proto_register_field_array(d->proto, hf, array_length(hf));
    proto_register_subtree_array(d->ett_array, array_length(d->ett_array));

    fprintf(stderr, "done registering %s\n", name);
}

void proto_register_one_handoff(int i)
{
    binpac_parser* parser = dissectors[i].parser;

    fprintf(stderr, "registering hand-of for %s\n", dissectors[i].name);

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    dissector_handle_t handle = new_create_dissector_handle(dissect_one, dissectors[i].proto);

    hlt_iterator_list j = hlt_list_begin(parser->ports, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parser->ports, &excpt, ctx);

    while ( ! (hlt_iterator_list_eq(j, end, &excpt, ctx) || excpt) ) {
        hlt_port p = *(hlt_port*) hlt_iterator_list_deref(j, &excpt, ctx);
        const char* tag = (p.proto == HLT_PORT_UDP) ? "udp.port" : "tcp.port";
        dissector_add_uint(tag, p.port, handle);
        j = hlt_iterator_list_incr(j, &excpt, ctx);
    }

    // FIXME: Remove.
//    if ( i == 0 ) dissector_add_uint("tcp.port", 12345, handle);

    fprintf(stderr, "done registering hand-of for %s\n", dissectors[i].name);
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
        proto_register_one(p, num_dissectors);
        i = hlt_iterator_list_incr(i, &excpt, ctx);
        num_dissectors++;
    }
}

void proto_register_all_binpac_handoff(void)
{
    fprintf(stderr, "Registering %d handoffs\n", num_dissectors);

    for ( int j = 0; j < num_dissectors; j++ )
        proto_register_one_handoff(j);

    fprintf(stderr, "Done registering handoffs\n");
}
