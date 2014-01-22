//
// A stand-alone driver for BinPAC++ generated parsers. This code has two
// modes of operation:
//
// (1) By default, it's a C program that can be built along with *.pac2 files
// using hilti-build.
//
// (2) If PAC_DRIVER_JIT is defined, it turns into a C++ program that
// compiles into a standalone binary that will JIT *.pac2 files at runtime.
//
// TODO: This is quite messy coe right now, needs a clean up.
//
// Note: This has bulk mode and threading stripped out. Shoudn't be hard to
// put it back in we need to, but it makes the code quite a bit more complex
// and not sure we still need it.

#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <sys/resource.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <stdint.h>

#undef DEBUG

#ifdef PAC_DRIVER_JIT
#include <hilti.h>
#include <binpac++.h>
#include <jit/libhilti-jit.h>

extern "C" {
    #include <libbinpac++.h>
}

#else

extern "C" {
    #include <libhilti.h>
    #include <libbinpac++.h>
}

struct Options {
    bool debug;
    bool optimize;
    unsigned int profile;

    Options() {
        debug = optimize = false;
        profile = 0;
    }
};

#endif

int driver_debug = 0;
int debug_hooks = 0;

binpac_parser* request = 0;
binpac_parser* reply = 0;

#ifdef PAC_DRIVER_JIT
std::set<string> cgdbg;
typedef hlt_list* (*binpac_parsers_func)(hlt_exception** excpt, hlt_execution_context* ctx);
binpac_parsers_func JitParsers = nullptr;
binpac::CompilerContext* PacContext = nullptr;
#endif

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);

        if ( hlt_exception_is_yield(excpt) || hlt_exception_is_termination(excpt) )
            exit(0);
        else {
            GC_DTOR(excpt, hlt_exception);
            GC_DTOR(request, hlt_BinPACHilti_Parser);
            GC_DTOR(reply, hlt_BinPACHilti_Parser);
            exit(1);
        }
    }
}

static void usage(const char* prog)
{
#ifdef PAC_DRIVER_JIT
    auto dbglist = binpac::Options().cgDebugLabels();
    auto dbgstr = util::strjoin(dbglist.begin(), dbglist.end(), "/");
#endif

    fprintf(stderr, "%s *.pac2 [options]\n\n", prog);
    fprintf(stderr, "  Options:\n\n");
    fprintf(stderr, "    -p <parser>[/<reply-parsers>]  Use given parser(s)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -g            Enable pac-driver's debug output\n");
    fprintf(stderr, "    -B            Enable BinPAC++ debugging hooks\n");
    fprintf(stderr, "    -i <n>        Feed input incrementally in chunks of size <n>\n");
    fprintf(stderr, "    -I            Add directory to import path.\n");
    fprintf(stderr, "    -l            Show available parsers\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    -P            Enable profiling\n");
#ifdef PAC_DRIVER_JIT
    fprintf(stderr, "    -d            Enable debug mode for JIT compilation\n");
    fprintf(stderr, "    -D <type>     Debug output during code generation; type can be %s\n", dbgstr.c_str());
    fprintf(stderr, "    -O            Optimize generated code.             [Default: off].\n");
    fprintf(stderr, "    -C            Use module cache.                    [Default: off].\n");
#endif
    fprintf(stderr, "\n");

    exit(1);
}

hlt_list* getParsers()
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

#ifdef PAC_DRIVER_JIT
    assert(JitParsers);
    return (*JitParsers)(&excpt, ctx);
#else
    return binpac_parsers(&excpt, ctx);
#endif
}

void listParsers()
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    hlt_list* parsers = getParsers();

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

        int first = 1;

        if ( p->ports ) {
            hlt_iterator_list j = hlt_list_begin(p->ports, &excpt, ctx);
            hlt_iterator_list end2 = hlt_list_end(p->ports, &excpt, ctx);

            while ( ! (hlt_iterator_list_eq(j, end2, &excpt, ctx) || excpt) ) {
                if ( first )
                    fputs("  [", stderr);
                else
                    fputs(", ", stderr);

                hlt_port p = *(hlt_port*) hlt_iterator_list_deref(j, &excpt, ctx);
                hlt_string s = hlt_port_to_string(&hlt_type_info_hlt_port, &p, 0, &excpt, ctx);
                hlt_string_print(stderr, s, 0, &excpt, ctx);
                GC_DTOR(s, hlt_string);

                hlt_iterator_list j2 = j;
                j = hlt_iterator_list_incr(j, &excpt, ctx);
                GC_DTOR(j2, hlt_iterator_list);
                first = 0;
            }

            GC_DTOR(j, hlt_iterator_list);
            GC_DTOR(end2, hlt_iterator_list);
        }

        if ( p->mime_types ) {
            hlt_iterator_list j = hlt_list_begin(p->mime_types, &excpt, ctx);
            hlt_iterator_list end2 = hlt_list_end(p->mime_types, &excpt, ctx);

            while ( ! (hlt_iterator_list_eq(j, end2, &excpt, ctx) || excpt) ) {
                if ( first )
                    fputs("  [", stderr);
                else
                    fputs(", ", stderr);

                hlt_string s = *(hlt_string*) hlt_iterator_list_deref(j, &excpt, ctx);
                hlt_string_print(stderr, s, 0, &excpt, ctx);
                GC_DTOR(s, hlt_string);

                hlt_iterator_list j2 = j;
                j = hlt_iterator_list_incr(j, &excpt, ctx);
                GC_DTOR(j2, hlt_iterator_list);
                first = 0;
            }

            GC_DTOR(j, hlt_iterator_list);
            GC_DTOR(end2, hlt_iterator_list);
        }

        if ( ! first )
            fputc(']', stderr);

        fputc('\n', stderr);

        GC_DTOR(p, hlt_BinPACHilti_Parser);

        hlt_iterator_list i2 = i;
        i = hlt_iterator_list_incr(i, &excpt, ctx);
        GC_DTOR(i2, hlt_iterator_list);
    }

    GC_DTOR(i, hlt_iterator_list);
    GC_DTOR(end, hlt_iterator_list);

    check_exception(excpt);

    if ( ! count )
        fprintf(stderr, "    None.\n");

    fputs("\n", stderr);

    GC_DTOR(parsers, hlt_list);
}

static binpac_parser* findParser(const char* name)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    hlt_list* parsers = getParsers();

    hlt_string hname = hlt_string_from_asciiz(name, &excpt, ctx);

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    binpac_parser* result = 0;

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);

        if ( hlt_string_cmp(hname, p->name, &excpt, ctx) == 0 ) {
            result = p;
            break;
        }

        GC_DTOR(p, hlt_BinPACHilti_Parser);

        hlt_iterator_list j = i;
        i = hlt_iterator_list_incr(i, &excpt, ctx);
        GC_DTOR(j, hlt_iterator_list);
    }


    GC_DTOR(i, hlt_iterator_list);
    GC_DTOR(end, hlt_iterator_list);
    GC_DTOR(parsers, hlt_list);
    GC_DTOR(hname, hlt_string);

    check_exception(excpt);

    return result;
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

        int8_t* copy = (int8_t*) hlt_malloc(n);
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

static void dump_memstats()
{
    if ( ! driver_debug )
        return;

    hlt_memory_stats stats = hlt_memory_statistics();
    uint64_t heap = stats.size_heap / 1024 / 1024;
    uint64_t alloced = stats.size_alloced / 1024 / 1024;
    uint64_t total_refs = stats.num_refs;
    uint64_t current_allocs = stats.num_allocs - stats.num_deallocs;

    fprintf(stderr, "--- pac-driver stats: "
                    "%" PRIu64 "M heap, "
                    "%" PRIu64 "M alloced, "
                    "%" PRIu64 " allocations, "
                    "%" PRIu64 " totals refs"
                    "\n",
            heap, alloced, current_allocs, total_refs);
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

        if ( driver_debug )
            fprintf(stderr, "--- pac-driver: starting parsing single input chunk.\n");

        void *pobj = (*p->parse_func)(input, 0, &excpt, ctx);

        if ( driver_debug )
            fprintf(stderr, "--- pac-driver: done parsing single input chunk.\n");

        GC_DTOR_GENERIC(&pobj, p->type_info);
        GC_DTOR(input, hlt_bytes);
        GC_DTOR(cur, hlt_iterator_bytes);
        check_exception(excpt);
        dump_memstats();
        return;
    }

    // Feed incrementally.

    hlt_bytes* chunk1;
    hlt_bytes* chunk2;
    hlt_iterator_bytes end = hlt_bytes_end(input, &excpt, ctx);
    hlt_iterator_bytes cur_end;
    int8_t done = 0;
    hlt_exception* resume = 0;

    hlt_bytes* incr_input = hlt_bytes_new(&excpt, ctx);

    dump_memstats();

    while ( ! done ) {
        cur_end = hlt_iterator_bytes_incr_by(cur, chunk_size, &excpt, ctx);
        done = hlt_iterator_bytes_eq(cur_end, end, &excpt, ctx);

        chunk1 = hlt_bytes_sub(cur, cur_end, &excpt, ctx);
        chunk2 = hlt_bytes_copy(chunk1, &excpt, ctx); // FIXME: Need?
        hlt_bytes_append(incr_input, chunk1, &excpt, ctx);
        GC_DTOR(chunk1, hlt_bytes);
        GC_DTOR(chunk2, hlt_bytes);

        if ( done )
            hlt_bytes_freeze(incr_input, 1, &excpt, ctx);

        int frozen = hlt_bytes_is_frozen(incr_input, &excpt, ctx);

        check_exception(excpt);

        if ( ! resume ) {
            if ( driver_debug )
                fprintf(stderr, "--- pac-driver: starting parsing (eod=%d).\n", frozen);

            void *pobj = (*p->parse_func)(incr_input, 0, &excpt, ctx);
            GC_DTOR_GENERIC(&pobj, p->type_info);
        }

        else {
            if ( driver_debug )
                fprintf(stderr, "--- pac-driver: resuming parsing (eod=%d, excpt=%p).\n", frozen, resume);

            void *pobj = (*p->resume_func)(resume, &excpt, ctx);
            GC_DTOR_GENERIC(&pobj, p->type_info);
        }

        if ( driver_debug )
            dump_memstats();

        if ( excpt ) {
            if ( hlt_exception_is_yield(excpt) ) {
                if ( driver_debug )
                    fprintf(stderr, "--- pac-driver: parsing yielded.\n");

                resume = excpt;
                excpt = 0;
            }

            else {
                GC_DTOR(cur, hlt_iterator_bytes);
                GC_DTOR(cur_end, hlt_iterator_bytes);
                GC_DTOR(end, hlt_iterator_bytes);
                GC_DTOR(input, hlt_bytes);
                GC_DTOR(incr_input, hlt_bytes);
                check_exception(excpt);
            }
        }

        else if ( ! done ) {
            if ( driver_debug )
                fprintf(stderr, "pac-driver: end of input reached even though more could be parsed.");

            GC_DTOR(cur_end, hlt_iterator_bytes);
            break;
        }

        GC_DTOR(cur, hlt_iterator_bytes);
        cur = cur_end;
    }

    GC_DTOR(end, hlt_iterator_bytes);
    GC_DTOR(input, hlt_bytes);
    GC_DTOR(incr_input, hlt_bytes);
    GC_DTOR(cur, hlt_iterator_bytes);
}

#ifdef PAC_DRIVER_JIT

// C++ code for JIT version.

bool jitPac2(const std::list<string>& pac2, std::shared_ptr<binpac::Options> options)
{
    hilti::init();
    binpac::init();

    PacContext = new binpac::CompilerContext(options);

    std::list<llvm::Module* > llvm_modules;

    for ( auto p : pac2 ) {
        auto llvm_module = PacContext->compile(p);

        if ( ! llvm_module ) {
            fprintf(stderr, "compiling %p failed\n", p.c_str());
            return false;
        }

        llvm_modules.push_back(llvm_module);
    }

    auto linked_module = PacContext->linkModules("<jit analyzers>", llvm_modules);

    if ( ! linked_module ) {
        fprintf(stderr, "linking failed\n");
        return false;
    }

    auto ee = PacContext->hiltiContext()->jitModule(linked_module);

    if ( ! ee ) {
        fprintf(stderr, "jit failed");
        return false;
    }

    // TODO: This should be done by jitModule, which however then needs to
    // move into hilti-jit.
    hlt_init_jit(PacContext->hiltiContext(), linked_module, ee);
    binpac_init();
    binpac_init_jit(PacContext->hiltiContext(), linked_module, ee);

    auto func = PacContext->hiltiContext()->nativeFunction(linked_module, ee, "binpac_parsers");

    if ( ! func ) {
        fprintf(stderr, "jitBinpacParser error: no function 'binpac_parsers'");
        return false;
    }

    JitParsers = (binpac_parsers_func)func;

    return true;
}

#endif

int main(int argc, char** argv)
{
    int chunk_size = 0;
    int list_parsers = false;
    const char* parser = 0;

    const char* progname = argv[0];

#ifdef PAC_DRIVER_JIT
    auto options = std::make_shared<binpac::Options>();
#else
    ::Options _options;
    ::Options* options = &_options;
#endif

    char ch;
    while ((ch = getopt(argc, argv, "i:p:t:v:s:dOBhD:UlTPgCI:")) != -1) {

        switch (ch) {

          case 'i':
            chunk_size = atoi(optarg);
            break;

          case 'p':
            parser = optarg;
            break;

          case 'd':
            options->debug = true;
            break;

          case 'g':
            ++driver_debug;
            break;

          case 'B':
            debug_hooks = 1;
            break;

          case 'P':
            ++options->profile;
            break;

          case 'l':
            list_parsers = true;
            break;

         case 'I':
            options->libdirs_pac2.push_back(optarg);
            break;

#ifdef PAC_DRIVER_JIT
         case 'D':
            options->cg_debug.insert(optarg);
            break;

         case 'O':
            options->optimize = true;
            break;

         case 'C':
            options->module_cache = ".cache";
            break;
#endif

          case 'h':
            // Fall-through.
          default:
            usage(progname);
        }
    }

    argc -= optind;
    argv += optind;

    hlt_config cfg = *hlt_config_get();

    if ( options->profile ) {
        fprintf(stderr, "Enabling profiling ...\n");
        cfg.profiling = 1;
    }

    hlt_config_set(&cfg);

#ifdef PAC_DRIVER_JIT
    std::list<string> pac2;

    for ( int i = 0; i < argc; i++ )
        pac2.push_back(argv[i]);

    if ( ! pac2.size() )
        usage(progname);

    options->jit = true;

    if ( ! jitPac2(pac2, options) )
        return 1;
#else
    hlt_init();
    binpac_init();
#endif

    if ( list_parsers ) {
        listParsers();
        return 0;
    }

    hlt_execution_context* ctx = hlt_global_execution_context();

    binpac_enable_debugging(debug_hooks);

    if ( ! parser ) {
        hlt_exception* excpt = 0;

        hlt_list* parsers = getParsers();

        // If we have exactly one parser, that's the one we'll use.
        if ( hlt_list_size(parsers, &excpt, ctx) == 1 ) {
            hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
            request = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);
            assert(request);
            GC_CCTOR(request, hlt_BinPACHilti_Parser);
            reply = request;
            GC_DTOR(i, hlt_iterator_list);
            GC_DTOR(parsers, hlt_list);
        }

        else {
            // If we don't have any parsers, we do nothing and just exit
            // normally.
            int64_t size = hlt_list_size(parsers, &excpt, ctx);
            GC_DTOR(parsers, hlt_list);

            if ( size == 0 )
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

        else {
            GC_CCTOR(request, hlt_BinPACHilti_Parser);
            reply = request;
        }
    }

    assert(request && reply);

    if ( options->profile ) {
        hlt_exception* excpt = 0;
        hlt_string profiler_tag = hlt_string_from_asciiz("app-total", &excpt, hlt_global_execution_context());
        hlt_profiler_start(profiler_tag, Hilti_ProfileStyle_Standard, 0, 0, &excpt, hlt_global_execution_context());
        GC_DTOR(profiler_tag, hlt_string);
    }

    parseSingleInput(request, chunk_size);

    if ( options->profile ) {
        hlt_exception* excpt = 0;
        hlt_string profiler_tag = hlt_string_from_asciiz("app-total", &excpt, hlt_global_execution_context());
        hlt_profiler_stop(profiler_tag, &excpt, hlt_global_execution_context());
        GC_DTOR(profiler_tag, hlt_string);
    }

    GC_DTOR(request, hlt_BinPACHilti_Parser);
    GC_DTOR(reply, hlt_BinPACHilti_Parser);

    exit(0);
}
