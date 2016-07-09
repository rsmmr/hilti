//
// Small wrapper for generating C prototypes and LLVM-level type information
// for a HILTI module.
//

#include <getopt.h>
#include <iostream>
#include <fstream>

#include <hilti.h>


extern "C" {
#include <libhilti/globals.h>

__hlt_global_state* __hlt_globals()
{
    return 0;
}

void* __hlt_runtime_state_get()
{
    return 0;
}

void hlt_done()
{
}

void hlt_global_state_dump(FILE* f)
{
}

void __hlt_runtime_state_set(void *state)
{
}

}

const char* Name = "hiltip";

static struct option long_options[] = {
    { "ast",     no_argument, 0, 'A' },
    { "debug",   no_argument, 0, 'd' },
    { "cgdebug", required_argument, 0, 'D' },
    { "help",    no_argument, 0, 'h' },
    { "output",  required_argument, 0, 'o' },
    { "import",  required_argument, 0, 'I' },
    { "prototypes", no_argument, 0, 'P' },
    { "type-info", no_argument, 0, 'T' },
    { "opt", no_argument, 0, 'O' },
    { 0, 0, 0, 0 }
};

void usage()
{
    auto dbglist = hilti::Options().cgDebugLabels();
    auto dbgstr = util::strjoin(dbglist.begin(), dbglist.end(), "/");

    std::cerr << "Usage: " << Name << " [-P|-T] [options] <file.hlt>\n"
            "\n"
            "Options:\n"
            "\n"
            "  -A | --ast            Dump intermediary HILTI ASTs to stderr.\n"
            "  -D | --cgdebug <type> Debug output during code generation; type can be " << dbgstr << ".\n"
            "  -I | --import <dir>   Search library files in <dir>. Can be given multiple times.\n"
            "  -O | --opt            Assume that optimized code will be generated. [Default: off].\n"
            "  -P | --prototypes     Generate C prototypes for HILTI module.\n"
            "  -T | --type-info      Output LLVM bitcode with type information.\n"
            "  -d | --debug          Debug level. Each time increases level.       [Default: 0]\n"
            "  -h | --help           Print usage information.\n"
            "  -o | --output <file>  Specify output file.                          [Default: stdout].\n"
            "  -v | --version        Print version information.\n"
            "\n";
}

void version()
{
    std::cerr << Name << " v" << hilti::version() << std::endl;
}

int main(int argc, char** argv)
{
    bool dump_ast = false;
    bool output_type_info = false;
    bool output_prototypes = false;
    std::string output;

    shared_ptr<hilti::Options> options = std::make_shared<hilti::Options>();

    while ( true ) {
        int c = getopt_long(argc, argv, "PTOAD:dghvo:I:", long_options, 0);

        if ( c < 0 )
            break;

        switch (c) {
         case 'A':
            dump_ast = true;
            break;

         case 'd':
            options->debug = true;
            break;

         case 'D':
            options->cg_debug.insert(optarg);
            break;

         case 'o':
            output = optarg;
            break;

         case 'I':
            options->libdirs_hlt.push_back(optarg);
            break;

         case 'O':
            options->optimize = true;
            break;

         case 'v':
            ::version();
            return 0;

         case 'h':
            usage();
            return 0;

         case 'P':
            output_prototypes = true;
            break;

         case 'T':
            output_type_info = true;
            break;

         default:
            usage();
            return 1;
        }

    }

    if ( optind != (argc - 1) ) {
        usage();
        return 1;
    }

    if ( ! (output_type_info || output_prototypes) ) {
        usage();
        return 1;
    }

    if ( output_type_info && output_prototypes ) {
        usage();
        return 1;
    }

    std::string input = argv[optind];

    if ( ! output.size() )
        output = "/dev/stdout";

    hilti::init();

    auto ctx = std::make_shared<hilti::CompilerContext>(options);
    auto module = ctx->loadModule(input);

    if ( ! module ) {
        std::cerr << "aborting due to HILTI error" << std::endl;
        return 1;
    }

    if ( dump_ast )
        ctx->dump(module, std::cerr);

    std::ofstream out(output, std::ios::out | std::ios::trunc);

    if ( ! out.is_open() ) {
        std::cerr << "cannot open output file" << std::endl;
        return 1;
    }

    if ( output_prototypes )
        ctx->generatePrototypes(module, out);

    if ( output_type_info ) {
        std::unique_ptr<llvm::Module> llvm_module(ctx->compile(module));

        if ( ! llvm_module ) {
            std::cerr << "aborting due to code generation error" << std::endl;
            return 1;
        }

        ctx->writeBitcode(llvm_module.get(), out);
    }

    return 0;
}

