
#include <getopt.h>
#include <iostream>
#include <fstream>

#include <binpac++.h>
#include <hilti.h>
#include <util.h>

using namespace std;

const char* Name = "binpac";

bool dump_ast = false;
bool cfg = false;
bool output_binpac = false;
bool output_llvm = false;
bool output_prototypes = false;
bool add_stdlibs = false;

string output = "/dev/stdout";

static struct option long_options[] = {
    { "ast",     no_argument, 0, 'A' },
    { "debug",   no_argument, 0, 'd' },
    { "cgdebug", no_argument, 0, 'D' },
    { "help",    no_argument, 0, 'h' },
    { "print",   no_argument, 0, 'p' },
    { "print-always",   no_argument, 0, 'W' },
    { "cfg",   no_argument, 0, 'C' },
    { "prototypes", no_argument, 0, 'P' },
    { "output",  required_argument, 0, 'o' },
    { "version", no_argument, 0, 'v' },
    { "llvm", no_argument, 0, 'l' },
    { "optimize", no_argument, 0, 'O' },
    { "add-stdlibs", no_argument, 0, 's' },
    { "compose", no_argument, 0, 'c' },
    { 0, 0, 0, 0 }
};

void usage()
{
    auto dbglist = binpac::Options().cgDebugLabels();
    auto dbgstr = util::strjoin(dbglist.begin(), dbglist.end(), "/");

    cerr << "Usage: " << Name << " [options] <input.pac2>\n"
            "\n"
            "Options:\n"
            "\n"
            "  -A | --ast            Dump intermediary ASTs to stderr.\n"
            "  -c | --compose        Generate composing code as well\n"
            "  -C | --cfg            When outputting HILTI code, include control/data flow information.\n"
            "  -d | --debug          Debug level for the generated code. Each time increases level. [Default: 0]\n"
            "  -D | --cgdebug <type> Debug output during code generation; type can be " << dbgstr << ".\n"
            "  -h | --help           Print usage information.\n"
            "  -I | --import <dir>   Add directory to import path.\n"
            "  -n | --no-validate    Do not validate resulting BinPAC++ or HILTI ASTs (for debugging only).\n"
            "  -o | --output <file>  Specify output file.                    [Default: stdout].\n"
            "  -p | --print          Just output all parsed BinPAC++ code again.\n"
            "  -W | --print-always   Like -p, but don't verify correctness first.\n"
            "  -l | --llvm           Output the final LLVM code.\n"
            "  -O | --optimize       Optimize generated code (for -l         [Default: off].\n"
            "  -P | --prototypes     Generate C API prototypes for generated module.\n"
            "  -s | --add-stdlibs    Add standard HILTI runtime libraries (for -l).\n"
            "  -t | --type <t>       Type of code to generate: parse/compose/both [Default: parse].\n"
            "\n";
}

void version()
{
    cerr << Name << " v" << binpac::version() << endl;
}

void error(const string& file, const string& msg)
{
    if ( file.size() )
        cerr << util::basename(file) << ": ";

    cerr << msg << endl;
    exit(1);
}

int main(int argc, char** argv)
{
    shared_ptr<binpac::Options> options = std::make_shared<binpac::Options>();

    options->generate_parsers = true;
    options->generate_composers = false;

    while ( true ) {
        int c = getopt_long(argc, argv, "AcCdD:o:nOPWlspI:vht:", long_options, 0);

        if ( c < 0 )
            break;

        switch (c) {
         case 'A':
            dump_ast = true;
            break;

         case 'c':
            options->generate_composers = true;
            break;

         case 'C':
            cfg = true;
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

         case 'n':
            options->verify = false;
            break;

         case 'W':
            output_binpac = true;
            options->verify = false;
            break;

         case 'p':
            output_binpac = true;
            break;

         case 'P':
            output_prototypes = true;
            break;

         case 'l':
            output_llvm = true;
            break;

         case 's':
            add_stdlibs = true;
            break;

         case 'O':
            options->optimize = true;
            break;

         case 'I':
            options->libdirs_pac2.push_back(optarg);
            break;

         case 'v':
            ::version();
            return 0;

         case 't':
            if ( strcmp(optarg, "parse") == 0 ) {
                options->generate_parsers = true;
                options->generate_composers = false;
            }

            else if ( strcmp(optarg, "compose") == 0 ) {
                options->generate_parsers = false;
                options->generate_composers = true;
            }

            else if ( strcmp(optarg, "both") == 0 ) {
                options->generate_parsers = true;
                options->generate_composers = true;
            }

            else {
                error("", "-t argument must be 'parse', 'compose', or 'both.'");
            }

            break;

         case 'h':
            usage();
            return 0;

         default:
            usage();
            return 1;
        }

    }

    if ( optind == argc )
        error("", "No input file given.");

    if ( optind - argc > 1 )
        error("", "Two many input file given.");

    string input = argv[optind];

    ofstream out;
    out.open(output, ios::out | ios::trunc);

    if ( ! out.good() )
        error(output, "Cannot open file for output.");

    hilti::init();
    binpac::init();

    auto ctx = std::make_shared<binpac::CompilerContext>(options);
    auto module = ctx->load(input);

    if ( ! module ) {
        error(input, "Aborting due to input error.");
        return 1;
    }

    if ( dump_ast ) {
        ctx->dump(module, cerr);
        cerr << std::endl;
    }

    if ( output_binpac )
        return ctx->print(module, out);

    shared_ptr<hilti::Module> hilti_module = 0;
    auto llvm_module = ctx->compile(module, &hilti_module, ! output_llvm);

    if ( hilti_module ) {

        if ( dump_ast ) {
            hilti_module->dump(cerr);
            cerr << std::endl;
        }

        if ( output_prototypes ) {
            ctx->hiltiContext()->generatePrototypes(hilti_module, out);
            return 0;
        }

        if ( ctx->options().cgDebugging("scopes") ) {
            hilti::passes::ScopePrinter scope_printer(cerr);

            if ( ! scope_printer.run(hilti_module) )
                return 1;
        }
    }

    if ( ! hilti_module ) {
        error(input, "Aborting due to compilation error.");
        return 1;
    }

    if ( output_llvm ) {

        if ( ! llvm_module ) {
            error(input, "Aborting due to LLVM compilation error.");
            return 1;
        }

        std::list<llvm::Module *> mods = { llvm_module };
        auto linked_module = ctx->linkModules(input, mods,
                                              std::list<string>(),
                                              path_list(), path_list(),
                                              add_stdlibs);

        if ( ! linked_module ) {
            error(input, "Aborting due to link error.");
            return 1;
        }

        if ( ! ctx->hiltiContext()->printBitcode(linked_module, std::cout) ) {
            error(input, "Aborting due to LLVM problem.");
            return 1;
        }

        return 0;
    }

    if ( ! ctx->hiltiContext()->print(hilti_module, out, cfg) ) {
        error(input, "Aborting due to HILTI printer error.");
        return 1;
    }

    return 0;
}
