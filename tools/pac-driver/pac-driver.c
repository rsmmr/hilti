// $Id$
//
// A stand-alone driver for BinPAC++ generated parsers.

int sleep = 0;

#include <libhilti.h>

#include <stdio.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <sys/resource.h>

#include <binpac.h>

int debug = 0;
int debug_hooks = 0;

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);

        if ( excpt->type == &hlt_exception_yield )
            exit(0);
        else
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
    fprintf(stderr, "    -P            Enable profiling\n");
    fprintf(stderr, "\n");
#ifdef HTTP_THREADED
    fprintf(stderr, "    -T            Enabling bulk mode with threading; works only for HTTP currently.\n");
    fprintf(stderr, "    -t <n>        The number of HILTI native threads to use. (default: 5)\n");
    fprintf(stderr, "    -v <n>        The number of HILTI virtual threads to use. (default: 50)\n");
    fprintf(stderr, "    -a <str>      Specificy thread affinity as comma-separated 'name:core' pairs (default: unset)\n");
#endif
    fprintf(stderr, "\n");

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    hlt_list* parsers = binpac_parsers(&excpt, ctx);

    if ( hlt_list_size(parsers, &excpt, ctx) == 0 ) {
        fprintf(stderr, "  No parsers defined.\n\n");
        exit(1);
    }

    fprintf(stderr, "  Available parsers: \n\n");

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    int count = 0;

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        ++count;
        binpac_parser* p = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);

        fputs("    ", stderr);

        hlt_string_print(stderr, p->name, 0, &excpt, ctx);

        for ( int n = 25 - hlt_string_len(p->name, &excpt, ctx); n; n-- )
            fputc(' ', stderr);

        hlt_string_print(stderr, p->description, 0, &excpt, ctx);

        if ( p->mime_types && hlt_list_size(p->mime_types, &excpt, ctx) ) {
            fputs(" [", stderr);

            hlt_iterator_list j = hlt_list_begin(p->mime_types, &excpt, ctx);
            hlt_iterator_list end2 = hlt_list_end(p->mime_types, &excpt, ctx);

            int8_t first = 1;
            while ( ! (hlt_iterator_list_eq(j, end2, &excpt, ctx) || excpt) ) {
                if ( ! first )
                    fputs(", ", stderr);

                hlt_string s = *(hlt_string*) hlt_iterator_list_deref(j, &excpt, ctx);
                hlt_string_print(stderr, s, 0, &excpt, ctx);

                first = 0;
                j = hlt_iterator_list_incr(j, &excpt, ctx);
            }

            fputc(']', stderr);
        }

        fputc('\n', stderr);

        i = hlt_iterator_list_incr(i, &excpt, ctx);
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

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);

        if ( hlt_string_cmp(hname, p->name, &excpt, ctx) == 0 )
            return p;

        i = hlt_iterator_list_incr(i, &excpt, ctx);
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

        int8_t* copy = hlt_malloc(n);
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
    hlt_iterator_bytes cur = hlt_bytes_begin(input, &excpt, ctx);

    check_exception(excpt);

    if ( ! chunk_size ) {
        // Feed all input at once.
        hlt_bytes_freeze(input, 1, &excpt, ctx);
        (*p->parse_func)(input, 0, &excpt, ctx);
        check_exception(excpt);
        return;
    }

    // Feed incrementally.

    hlt_bytes* chunk;
    hlt_iterator_bytes end = hlt_bytes_end(input, &excpt, ctx);
    hlt_iterator_bytes cur_end;
    int8_t done = 0;
    hlt_exception* resume = 0;

    input = hlt_bytes_new(&excpt, ctx);

    while ( ! done ) {
        cur_end = hlt_iterator_bytes_incr_by(cur, chunk_size, &excpt, ctx);
        done = hlt_iterator_bytes_eq(cur_end, end, &excpt, ctx);

        chunk = hlt_bytes_sub(cur, cur_end, &excpt, ctx);
        hlt_bytes_append(input, chunk, &excpt, ctx);

        if ( done )
            hlt_bytes_freeze(input, 1, &excpt, ctx);

        int frozen = hlt_bytes_is_frozen(input, &excpt, ctx);

        check_exception(excpt);

        if ( ! resume ) {
            if ( debug )
                fprintf(stderr, "--- pac-driver: starting parsing (eod=%d).\n", frozen);

            (*p->parse_func)(input, 0, &excpt, ctx);
        }

        else {
            if ( debug )
                fprintf(stderr, "--- pac-driver: resuming parsing (eod=%d).\n", frozen);

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

static hlt_string current_flow = 0;

// May be called from generated driver. This is a hack to get the data into the hook ...
hlt_string hilti_get_flow(hlt_exception* excpt, hlt_execution_context* ctx)
{
    return current_flow;
}

Flow* bulkFeedPiece(binpac_parser* parser, Flow* flow, int eof, const char* data, int size, const char* fid, const char* t)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    // Make a copy of the data.
    int8_t* tmp = hlt_malloc(size);
    assert(tmp);
    memcpy(tmp, data, size);

    hlt_bytes* input = hlt_bytes_new_from_data(tmp, size, &excpt, ctx);

    check_exception(excpt);

    Flow* result = 0;

    if ( ! flow ) {
        flow = hlt_malloc(sizeof(Flow));
        flow->input = input;
        flow->resume = 0;
        flow->stopped = 0;
        result = flow;
    }
    else {

        if ( flow->stopped )
            // Error encountered earlier, just ignore further input.
            return 0;

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

    if ( eof )
        hlt_bytes_freeze(flow->input, 1, &excpt, ctx);

    check_exception(excpt);

    current_flow = hlt_string_from_asciiz(fid, &excpt, ctx);

    if ( ! flow->resume ) {
        if ( debug )
            fprintf(stderr, "--- pac-driver: starting parsing flow %s at %s.\n", fid, t);

        (*parser->parse_func)(input, 0, &excpt, ctx);
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
    int8_t* tmp = hlt_malloc(size);
    assert(tmp);
    memcpy(tmp, data, size);

    hlt_bytes* input = hlt_bytes_new_from_data(tmp, size, &excpt, ctx);
    hlt_bytes_freeze(input, 1, &excpt, ctx); // Always complete.

    check_exception(excpt);

    if ( debug )
        fprintf(stderr, "--- pac-driver: starting parsing packet for flow %s at %s.\n", fid, t);

    (*parser->parse_func)(input, 0, &excpt, ctx);

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

    int num_flows = 0;

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
        int payload = 0;

        if ( debug )
            fprintf(stderr, "--- pac-driver: kind=%c dir=%c size=%d fid=|%s| t=|%s| known=%d\n", kind, dir, size, fid, ts, known_flow);

        if ( known_flow && kind == 'G' ) {
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

            payload = 1;
        }

        if ( ! known_flow ) {
            int ret;
            i = kh_put(Flow, hash, strdup(fid), &ret);
            kh_value(hash, i) = 0;

            ++num_flows;
        }

        if ( payload && ! ignore ) {
            binpac_parser* p = (dir == '>') ? request_parser : reply_parser;

            void* result = 0;

            if ( kind == 'D' )
                result = bulkFeedPiece(p, kh_value(hash, i), eof, buffer, size, fid, ts);
            else
                result = bulkFeedPacket(p, kh_value(hash, i), eof, buffer, size, fid, ts);

            if ( result )
                kh_value(hash, i) = result;
        }

        if ( eof ) {
            // Flush state (gc will take care of actually releasing the memory.
            if ( kh_value(hash, i) )
                kh_value(hash, i) = 0;

            kh_del(Flow, hash, i);
        }

        // Output a state summary in regular intervals.
        static int cnt = 0;

        if ( ++cnt % 10000 == 0 ) {
            struct rusage usage;
            getrusage(RUSAGE_SELF, &usage);
            fprintf(stderr, "--- pac-driver bulk state: %d total flows, %d in memory at %s, maxrss %lu\n", num_flows, kh_size(hash), ts, usage.ru_maxrss);
        }
    }

}

// #include "http.h"

struct Packet {
    uint32_t mask;
    hlt_time t;
    hlt_addr orig_h;
    hlt_port orig_p;
    hlt_addr resp_h;
    hlt_port resp_p;
    int8_t is_orig;
    int8_t eof;
    hlt_bytes* data;
};

#ifdef HTTP_THREADED

// This can handle only HTTP at the moment and sources processing out to http.hlt.
void parseBulkInputThreaded()
{
    hlt_threading_start();

    hlt_execution_context* ctx = hlt_global_execution_context();

    static char buffer[65536];
    static char fid[256];
    static char ts[256];
    static char orig_h[256];
    static char resp_h[256];
    static char orig_p[256];
    static char resp_p[256];
    int cnt = 0;

    uint64_t start = 0;

    hlt_exception* excpt = 0;
    hlt_bytes* empty = hlt_bytes_new_from_data((int8_t*)"", 0, &excpt, ctx);

    while ( true ) {
        char* p = fgets(buffer, sizeof(buffer), stdin);
        char* e = p + strlen(buffer);

        if ( feof(stdin) )
            break;

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
        int eof = 0;
        int payload = 0;
        int ignore = 0;

        char* f = &p[6];
        while ( *f++ != ' ' );
        sscanf(f, "%[^:]:%[^-]-%[^:]:%s", orig_h, orig_p, resp_h, resp_p);
        char* ts = f;
        while ( *ts++ != ' ' );

        hlt_time t = hlt_time_from_timestamp(strtod(ts, 0));

        if ( debug )
            fprintf(stderr, "--- pac-driver: kind=%c dir=%c size=%d orig_h=%s orig_p=%s resp_h=%s resp_p=%s t=|%s|\n", kind, dir, size, orig_h, orig_p, resp_h, resp_p, ts);

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

            payload = 1;
        }

        if ( payload || eof ) {
            struct Packet p;
            p.mask = 255;
            p.orig_h = hlt_addr_from_asciiz(orig_h, &excpt, ctx);
            p.orig_p = hlt_port_from_asciiz(orig_p, &excpt, ctx);
            p.resp_h = hlt_addr_from_asciiz(resp_h, &excpt, ctx);
            p.resp_p = hlt_port_from_asciiz(resp_p, &excpt, ctx);
            p.is_orig = (dir == '>');
            p.eof = eof;

            int8_t* tmp = 0;
            if ( size ) {
                tmp = hlt_malloc(size);
                assert(tmp);
                memcpy(tmp, buffer, size);
                p.data = hlt_bytes_new_from_data(tmp, size, &excpt, ctx);
            }
            else
                p.data = empty;

            http_newChunk(&p, &excpt);

            if ( excpt )
                hlt_exception_print_uncaught(excpt, __hlt_global_execution_context);
        }

        if ( start == 0 ) {
            hlt_exception* excpt = 0;
            start = hlt_time_nsecs(t, &excpt, ctx);
        }

        ++cnt;

#if 1
        if ( sleep && (cnt % sleep == 0) ) {
            hlt_util_nanosleep(1);
        }

#else
        if ( cnt % 10 == 0 ) {
            hlt_exception* excpt = 0;
            double load = hlt_threading_load(&excpt);

            if ( load > 0.2 && sleep )
                hlt_util_nanosleep(sleep);
        }
#endif

        if ( cnt % 10000 == 0 ) {
            hlt_exception* excpt = 0;
            uint64_t current = hlt_time_nsecs(t, &excpt, ctx);
            fprintf(stderr, "--- pac-driver bulk state: gc heap size %lu after %.2f hours, HILTI load %.2f\n", GC_get_heap_size(), (double)(current-start)/1e9/60.0/60.0, hlt_threading_load(&excpt));
        }

    }

    fprintf(stderr, "--- pac-driver waiting for threads to finish ...\n");
    hilti_wait_for_threads();
    fprintf(stderr, "--- pac-driver done ...\n");

    hlt_threading_stop(&excpt);

    if ( excpt )
        hlt_exception_print_uncaught(excpt, __hlt_global_execution_context);
}

#endif

int main(int argc, char** argv)
{
    int chunk_size = 0;
    int bulk = 0;
    int threading = 0;
    int threads = 5;
    int vthreads = 50;
    int profiling = 0;
    const char* parser = 0;
    const char* affinity = 0;

    hlt_init();
    binpac_init();

    char ch;
    while ((ch = getopt(argc, argv, "i:p:t:v:a:s:dBhUTP")) != -1) {

        switch (ch) {

          case 'i':
            chunk_size = atoi(optarg);
            break;

          case 'p':
            parser = optarg;
            break;

          case 'a':
            affinity = strdup(optarg);
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

          case 'T':
            threading = 1;
            break;

          case 'P':
            profiling = 1;
            break;

          case 't':
            threads = atoi(optarg);
            break;

          case 'v':
            vthreads = atoi(optarg);
            break;

          case 's':
            sleep = atoi(optarg);
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

    if ( threading && parser && strcmp(parser, "http") ) {
        fprintf(stderr, "error: threading mode supports only HTTP currently\n");
        exit(1);
    }

    hlt_config cfg = *hlt_config_get();
    cfg.num_workers = threads;
    cfg.vid_schedule_min = 1;
    cfg.vid_schedule_max = 1 + vthreads;

    if ( affinity )
        cfg.core_affinity = affinity;

    if ( profiling ) {
        fprintf(stderr, "Enabling profiling ...\n");
        cfg.profiling = 1;
    }

    hlt_config_set(&cfg);

    hlt_execution_context* ctx = hlt_global_execution_context();

    binpac_enable_debugging(debug_hooks);

#ifdef HTTP_THREADED
    if ( threading ) {
        parseBulkInputThreaded();
        return 0;
    }
#endif

    binpac_parser* request = 0;
    binpac_parser* reply = 0;

    if ( ! parser ) {
        hlt_exception* excpt = 0;
        hlt_list* parsers = binpac_parsers(&excpt, ctx);

        // If we have exactly one parser, that's the one we'll use.
        if ( hlt_list_size(parsers, &excpt, ctx) == 1 ) {
            hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
            request = reply = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);
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

    if ( profiling ) {
        hlt_exception* excpt = 0;
        hlt_string profiler_tag = hlt_string_from_asciiz("app-total", &excpt, hlt_global_execution_context());
        hlt_profiler_start(profiler_tag, Hilti_ProfileStyle_Standard, 0, 0, &excpt, hlt_global_execution_context());
        GC_DTOR(profiler_tag, hlt_string);
    }

    if ( ! bulk )
        parseSingleInput(request, chunk_size);
    else
        parseBulkInput(request, reply);

    if ( profiling ) {
        hlt_exception* excpt = 0;
        hlt_string profiler_tag = hlt_string_from_asciiz("app-total", &excpt, hlt_global_execution_context());
        hlt_profiler_stop(profiler_tag, &excpt, hlt_global_execution_context());
        GC_DTOR(profiler_tag, hlt_string);
    }

    exit(0);
}

