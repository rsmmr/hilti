// $Id$
//
// A stand-alone driver for BinPAC++ generated parsers.

#include <stdio.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>

#include <hilti.h>
#include <binpac.h>

int debug = 0;
int debug_hooks = 0;

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);
        exit(1);
    }
}

static void usage(const char* prog)
{
    fprintf(stderr, "%s [options]\n\n", prog);
    fprintf(stderr, "  Options:\n\n");
    fprintf(stderr, "    -p <parser>[/<reply-parsers>]  Use given parser(s)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -d            Enable pac-driver's debug output\n");
    fprintf(stderr, "    -B            Enable BinPAC++ debugging hooks\n");
    fprintf(stderr, "    -i  <n>       Feed input incrementally in chunks of size <n>\n");
    fprintf(stderr, "    -U            Enable bulk input mode\n");
    fprintf(stderr, "\n");

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    hlt_list* parsers = binpac_parsers(&excpt, ctx);

    if ( hlt_list_size(parsers, &excpt, ctx) == 0 ) {
        fprintf(stderr, "  No parsers defined.\n\n");
        exit(1);
    }

    fprintf(stderr, "  Available parsers: \n\n");

    hlt_list_iter i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_list_iter end = hlt_list_end(parsers, &excpt, ctx);

    int count = 0;

    while ( ! (hlt_list_iter_eq(i, end, &excpt, ctx) || excpt) ) {
        ++count;
        binpac_parser* p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt, ctx);

        fputs("    ", stderr);

        hlt_string_print(stderr, p->name, 0, &excpt, ctx);

        for ( int n = 20 - hlt_string_len(p->name, &excpt, ctx); n; n-- )
            fputc(' ', stderr);

        hlt_string_print(stderr, p->description, 1, &excpt, ctx);

        i = hlt_list_iter_incr(i, &excpt, ctx);
    }

    check_exception(excpt);

    if ( ! count )
        fprintf(stderr, "    None.\n");

    fputs("\n", stderr);

    exit(1);
}

static binpac_parser* findParser(const char* name)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    hlt_list* parsers = binpac_parsers(&excpt, ctx);

    hlt_string hname = hlt_string_from_asciiz(name, &excpt, ctx);

    hlt_list_iter i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_list_iter end = hlt_list_end(parsers, &excpt, ctx);

    while ( ! (hlt_list_iter_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_list_iter_deref(i, &excpt, ctx);

        if ( hlt_string_cmp(hname, p->name, &excpt, ctx) == 0 )
            return p;

        i = hlt_list_iter_incr(i, &excpt, ctx);
    }

    check_exception(excpt);
    return 0;
}

hlt_bytes* readAllInput()
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    hlt_bytes* input = hlt_bytes_new(&excpt, ctx);
    check_exception(excpt);

    int8_t buffer[256];

    while ( ! excpt ) {
        size_t n = fread(buffer, 1, sizeof(buffer), stdin);

        int8_t* copy = hlt_gc_malloc_atomic(n);
        memcpy(copy, buffer, n);
        hlt_bytes_append_raw(input, copy, n, &excpt, ctx);

        if ( feof(stdin) )
            break;

        if ( ferror(stdin) ) {
            fprintf(stderr, "error while reading from stdin\n");
            exit(1);
        }
    }

    check_exception(excpt);

    return input;

}

void parseSingleInput(binpac_parser* p, int chunk_size)
{
    hlt_bytes* input = readAllInput();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    hlt_bytes_pos cur = hlt_bytes_begin(input, &excpt, ctx);

    check_exception(excpt);

    if ( ! chunk_size ) {
        // Feed all input at once.
        hlt_bytes_freeze(input, 1, &excpt, ctx);
        (*p->parse_func)(cur, 0, &excpt, ctx);
        check_exception(excpt);
        return;
    }

    // Feed incrementally.

    hlt_bytes* chunk;
    hlt_bytes_pos end = hlt_bytes_end(input, &excpt, ctx);
    hlt_bytes_pos cur_end;
    int8_t done = 0;
    hlt_exception* resume = 0;

    input = hlt_bytes_new(&excpt, ctx);

    while ( ! done ) {
        cur_end = hlt_bytes_pos_incr_by(cur, chunk_size, &excpt, ctx);
        done = hlt_bytes_pos_eq(cur_end, end, &excpt, ctx);

        chunk = hlt_bytes_sub(cur, cur_end, &excpt, ctx);
        hlt_bytes_append(input, chunk, &excpt, ctx);

        if ( done )
            hlt_bytes_freeze(input, 1, &excpt, ctx);

        check_exception(excpt);

        if ( ! resume ) {
            if ( debug )
                fprintf(stderr, "--- pac-driver: starting parsing.\n");

            hlt_bytes_pos s = hlt_bytes_begin(input, &excpt, ctx);
            check_exception(excpt);

            (*p->parse_func)(s, 0, &excpt, ctx);
        }

        else {
            if ( debug )
                fprintf(stderr, "--- pac-driver: resuming parsing.\n");

            (*p->resume_func)(resume, &excpt, ctx);
        }

        if ( excpt ) {
            if ( excpt->type == &hlt_exception_yield ) {
                if ( debug )
                    fprintf(stderr, "--- pac-driver: parsing yielded.\n");

                resume = excpt;
                excpt = 0;
            }

            else
                check_exception(excpt);
        }

        else if ( ! done ) {
            if ( debug )
                fprintf(stderr, "pac-driver: end of input reached even though more could be parsed.");

            break;
        }

        cur = cur_end;
    }
}

static void input_error(const char* line)
{
    fprintf(stderr, "error reading input header: not in expected format.\n");
    exit(1);
}

static void parser_exception(const char*fid, hlt_exception* excpt)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    fprintf(stderr, "%s ", fid);
    hlt_exception_print_uncaught(excpt, ctx);
}

#include "khash.h"

typedef struct {
    hlt_bytes* input;
    hlt_exception* resume;
    int stopped;
} Flow;

KHASH_MAP_INIT_STR(Flow, Flow*)

Flow* bulkFeedPiece(binpac_parser* parser, Flow* flow, int eof, const char* data, int size, const char* fid, const char* t)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    // Make a copy of the data.
    int8_t* tmp = hlt_gc_malloc_atomic(size);
    assert(tmp);
    memcpy(tmp, data, size);

    hlt_bytes* input = hlt_bytes_new_from_data(tmp, size, &excpt, ctx);

    check_exception(excpt);

    Flow* result = 0;

    if ( ! flow ) {
        flow = hlt_gc_malloc_non_atomic(sizeof(Flow));
        flow->input = input;
        flow->resume = 0;
        flow->stopped = 0;
        result = flow;
    }
    else {

        if ( size && ! flow->resume ) {
            // Past parsing proceeded all the way through and we don't expect
            // further input.
            fprintf(stderr, "%s: error, no further input expected\n", fid);
            flow->stopped = 1;
            return 0;
        }

        assert(flow->input);
        hlt_bytes_append(flow->input, input, &excpt, ctx);
    }

    if ( flow->stopped )
        // Error encountered earlier, just ignore further input.
        return 0;

    if ( eof )
        hlt_bytes_freeze(flow->input, 1, &excpt, ctx);

    check_exception(excpt);

    if ( ! flow->resume ) {
        if ( debug )
            fprintf(stderr, "--- pac-driver: starting parsing flow %s at %s.\n", fid, t);

        hlt_bytes_pos begin = hlt_bytes_begin(input, &excpt, ctx);
        (*parser->parse_func)(begin, 0, &excpt, ctx);
    }

    else {
        if ( debug )
            fprintf(stderr, "--- pac-driver: resuming parsing flow %s at %s.\n", fid, t);

        (*parser->resume_func)(flow->resume, &excpt, ctx);
    }

    if ( excpt && excpt->type == &hlt_exception_yield ) {
        if ( debug )
            fprintf(stderr, "--- pac-driver: parsing yielded for flow %s at %s.\n", fid, t);

        flow->resume = excpt;

        excpt = 0;
    }

    else
        flow->resume = 0;

    if ( excpt ) {
        parser_exception(fid, excpt);
        flow->stopped = 1;
        return 0;
    }

    else if ( ! flow->resume ){
        if ( debug )
            fprintf(stderr, "--- pac-driver: parsing finished for flow %s at %s, no further input expected.\n", fid, t);
    }

    return result;
}

Flow* bulkFeedPacket(binpac_parser* parser, Flow* flow, int eof, const char* data, int size, const char* fid, const char* t)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    // Make a copy of the data.
    int8_t* tmp = hlt_gc_malloc_atomic(size);
    assert(tmp);
    memcpy(tmp, data, size);

    hlt_bytes* input = hlt_bytes_new_from_data(tmp, size, &excpt, ctx);
    hlt_bytes_freeze(input, 1, &excpt, ctx); // Always complete. 

    check_exception(excpt);

    if ( debug )
        fprintf(stderr, "--- pac-driver: starting parsing packet for flow %s at %s.\n", fid, t);

    hlt_bytes_pos begin = hlt_bytes_begin(input, &excpt, ctx);
    (*parser->parse_func)(begin, 0, &excpt, ctx);

    if ( excpt ) {
        // We also don't excpect any yield. 
        parser_exception(fid, excpt);
        return 0;
    }

    if ( debug )
        fprintf(stderr, "--- pac-driver: parsing of packet finished for flow %s at %s\n", fid, t);

    return 0;
}

void parseBulkInput(binpac_parser* request_parser, binpac_parser* reply_parser)
{
    static char buffer[65536];
    static char fid[256];
    static char ts[256];

    khash_t(Flow) *hash = kh_init(Flow);

    while ( true ) {
        char* p = fgets(buffer, sizeof(buffer), stdin);
        char* e = p + strlen(buffer);

        if ( feof(stdin) )
            return;

        if ( ! p ) {
            fprintf(stderr, "error reading input header: %s\n", strerror(errno));
            exit(1);
        }

        if ( e > p )
            *(e-1) = '\0'; // Kill the NL.

        if ( p[0] != '#' )
            input_error(buffer);

        // Format is
        //
        //    # <kind> <dir> <len> <fid> <time>

        char kind = p[2];
        char dir = p[4];

        int size = atoi(&p[6]);

        char* f = &p[6];
        while ( *f++ != ' ' );

        char* t = f;
        while ( *t++ != ' ' );
        *(t-1) = '\0';

        if ( t > e )
            input_error(buffer);

        strncpy(fid, f, sizeof(fid) - 3);
        fid[sizeof(fid)-1] = '0';

        strncpy(ts, t, sizeof(ts) - 3);
        ts[sizeof(ts)-1] = '0';

        // Make the flow ID uni-directional and look it up in the state table.
        int len = strlen(fid);
        fid[len] = '#';
        fid[len+1] = dir;
        fid[len+2] = '\0';
        khiter_t i = kh_get(Flow, hash, fid);

        int ignore = 0;
        int eof = 0;
        int known_flow = (i != kh_end(hash));

        if ( debug )
            fprintf(stderr, "--- pac-driver: kind=%c dir=%c size=%d fid=|%s| t=|%s| known=%d\n", kind, dir, size, fid, ts, known_flow);

        if ( kind == 'G' ) {
            // Can't handle gaps yet. Mark flow as to be ignored by setting parser to null.
            kh_value(hash, i) = 0;
            continue;
        }

        if ( known_flow && ! kh_value(hash, i) )
            ignore = 1;

        if ( kind == 'T' ) {
            size = 0;
            eof = 1;
        }

        if ( kind == 'D' || kind == 'U' ) {
            if ( size > sizeof(buffer) ) {
                fprintf(stderr, "error reading input: data chunk unexpected large (%d)\n", size);
                exit(1);
            }

            if ( fread(buffer, size, 1, stdin) != 1 ) {
                fprintf(stderr, "error reading input chunk: %s\n", strerror(errno));
                exit(1);
            }
        }

        if ( ! known_flow ) {
            int ret;
            i = kh_put(Flow, hash, strdup(fid), &ret);
            kh_value(hash, i) = 0;
        }

        if ( ! ignore ) {
            binpac_parser* p = (dir == '>') ? request_parser : reply_parser;

            void* result = 0;

            if ( kind == 'D' )
                result = bulkFeedPiece(p, kh_value(hash, i), eof, buffer, size, fid, ts);
            else
                result = bulkFeedPacket(p, kh_value(hash, i), eof, buffer, size, fid, ts);

            if ( result )
                kh_value(hash, i) = result;
        }

        if ( eof && kh_value(hash, i) ) {
            // Delete state we allocated. BinPAC state is cleanup via GC.
            free(kh_value(hash, i));
            kh_del(Flow, hash, i);
        }
    }
}

int main(int argc, char** argv)
{
    int chunk_size = 0;
    int bulk = 0;
    const char* parser = 0;

    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();

    char ch;
    while ((ch = getopt(argc, argv, "i:p:vdBhU")) != -1) {

        switch (ch) {

          case 'i':
            chunk_size = atoi(optarg);
            break;

          case 'p':
            parser = optarg;
            break;

          case 'd':
            ++debug;
            break;

          case 'B':
            debug_hooks = 1;
            break;

          case 'U':
            bulk = 1;
            break;

          case 'h':
            // Fall-through.
          default:
            usage(argv[0]);
        }
    }

    argc -= optind;
    argv += optind;

    if ( argc != 0 )
        usage(argv[0]);

    if ( chunk_size && bulk )
        fprintf(stderr, "warning: chunk size ignored in bulk mode\n");

    binpac_enable_debugging(debug_hooks);

    binpac_parser* request = 0;
    binpac_parser* reply = 0;

    if ( ! parser ) {
        hlt_exception* excpt = 0;
        hlt_list* parsers = binpac_parsers(&excpt, ctx);

        // If we have exactly one parser, that's the one we'll use.
        if ( hlt_list_size(parsers, &excpt, ctx) == 1 ) {
            hlt_list_iter i = hlt_list_begin(parsers, &excpt, ctx);
            request = reply = *(binpac_parser**) hlt_list_iter_deref(i, &excpt, ctx);
            assert(request);
        }

        else {
            // If we don't have any parsers, we do nothing and just exit
            // normally.
            if ( hlt_list_size(parsers, &excpt, ctx) == 0 )
                exit(0);

            fprintf(stderr, "no parser specified; see usage for list.\n");
            exit(1);
            }
    }

    else {
        char* reply_parser = strchr(parser, '/');
        if ( reply_parser )
            *reply_parser++ = '\0';

        request = findParser(parser);

        if ( ! request ) {
            fprintf(stderr, "unknown parser '%s'. See usage for list.\n", parser);
            exit(1);
        }

        if ( reply_parser ) {
            reply = findParser(reply_parser);

            if ( ! reply ) {
                fprintf(stderr, "unknown reply parser '%s'. See usage for list.\n", reply_parser);
                exit(1);
            }
        }

        else
            reply = request;
    }

    assert(request && reply);

    if ( ! bulk )
        parseSingleInput(request, chunk_size);
    else
        parseBulkInput(request, reply);

    exit(0);
}
