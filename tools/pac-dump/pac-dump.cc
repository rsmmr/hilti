#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <sys/resource.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <stdint.h>

#undef DEBUG

#include <hilti.h>
#include <binpac++.h>
#include <jit/libhilti-jit.h>

extern "C" {
    #include <libbinpac++.h>
}

extern void ascii_dump_object(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx);
extern void json_dump_object(const hlt_type_info* type, void* obj, int indent, hlt_exception** excpt, hlt_execution_context* ctx);

int json = 0;

binpac_parser* request = 0;

std::set<string> cgdbg;
typedef hlt_list* (*binpac_parsers_func)(hlt_exception** excpt, hlt_execution_context* ctx);
binpac_parsers_func JitParsers = nullptr;
binpac::CompilerContext* PacContext = nullptr;

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);

        if ( hlt_exception_is_yield(excpt) || hlt_exception_is_termination(excpt) )
            exit(0);
        else {
            GC_DTOR(excpt, hlt_exception, ctx);
            GC_DTOR(request, hlt_BinPACHilti_Parser, ctx);
            exit(1);
        }
    }
}

static void usage(const char* prog)
{
    auto dbglist = binpac::Options().cgDebugLabels();
    auto dbgstr = util::strjoin(dbglist.begin(), dbglist.end(), "/");

    fprintf(stderr, "%s *.pac2 [options]\n\n", prog);
    fprintf(stderr, "  Options:\n\n");
    fprintf(stderr, "    -p <parser>   Use given parser.\n");
    fprintf(stderr, "    -j            Output JSON.\n");
    fprintf(stderr, "    -I            Add directory to import path.\n");
    fprintf(stderr, "    -O            Optimize generated code.             [Default: off].\n");
    fprintf(stderr, "    -d            Enable debug mode for JIT compilation\n");
    fprintf(stderr, "    -f            Include field offsets in JSON output.\n");

    exit(1);
}

hlt_list* getParsers()
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    assert(JitParsers);
    return (*JitParsers)(&excpt, ctx);
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

        i = hlt_iterator_list_incr(i, &excpt, ctx);
    }

    check_exception(excpt);

    return result;
}

hlt_bytes* readAllInput()
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    hlt_bytes* input = hlt_bytes_new(&excpt, ctx);
    check_exception(excpt);

    int8_t buffer[4096];

    size_t offset = 0;

    while ( ! excpt ) {
        size_t n = fread(buffer, 1, sizeof(buffer), stdin);
        offset += n;

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

void parseSingleInput(binpac_parser* p)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    hlt_bytes* input = readAllInput();
    GC_CCTOR(input, hlt_bytes, ctx);

    check_exception(excpt);

    hlt_bytes_freeze(input, 1, &excpt, ctx);

    void *pobj = (*p->parse_func)(input, 0, &excpt, ctx);

    if ( json )
        json_dump_object(p->type_info, &pobj, 0, &excpt, ctx);
    else
        ascii_dump_object(p->type_info, &pobj, 0, &excpt, ctx);

    GC_DTOR_GENERIC(&pobj, p->type_info, ctx);
    GC_DTOR(input, hlt_bytes, ctx);
    check_exception(excpt);
    return;
}

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

int main(int argc, char** argv)
{
    const char* progname = argv[0];
    const char* parser = 0;

    auto options = std::make_shared<binpac::Options>();

    char ch;
    while ((ch = getopt(argc, argv, "OjdpfI:")) != -1) {

        switch (ch) {

          case 'p':
            parser = optarg;
            break;

         case 'f':
            options->record_offsets = true;
            break;

          case 'j':
            json = 1;
            break;

         case 'I':
            options->libdirs_pac2.push_back(optarg);
            break;

         case 'O':
            options->optimize = true;
            break;

          case 'd':
            options->debug = true;
            break;

          case 'h':
            // Fall-through.
          default:
            usage(progname);
        }
    }

    argc -= optind;
    argv += optind;

    hlt_config cfg = *hlt_config_get();
    hlt_config_set(&cfg);

    std::list<string> pac2;

    for ( int i = 0; i < argc; i++ )
        pac2.push_back(argv[i]);

    if ( ! pac2.size() )
        usage(progname);

    options->jit = true;

    if ( ! jitPac2(pac2, options) )
        return 1;

    hlt_execution_context* ctx = hlt_global_execution_context();

    if ( ! parser ) {
        hlt_exception* excpt = 0;

        hlt_list* parsers = getParsers();

        // If we have exactly one parser, that's the one we'll use.
        if ( hlt_list_size(parsers, &excpt, ctx) == 1 ) {
            hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
            request = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);
            assert(request);
        }

        else {
            // If we don't have any parsers, we do nothing and just exit
            // normally.
            int64_t size = hlt_list_size(parsers, &excpt, ctx);

            if ( size == 0 )
                exit(0);

            fprintf(stderr, "no parser specified; see usage for list.\n");
            exit(1);
        }
    }

    else {
        request = findParser(parser);

        if ( ! request ) {
            fprintf(stderr, "unknown parser '%s'. See usage for list.\n", parser);
            exit(1);
        }
    }

    assert(request);

    GC_CCTOR(request, hlt_BinPACHilti_Parser, ctx);
    parseSingleInput(request);
    GC_DTOR(request, hlt_BinPACHilti_Parser, ctx);

    exit(0);
}
