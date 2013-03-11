
#include <getopt.h>
#include <iostream>
#include <fstream>

#include <llvm/Module.h>

#include <hilti.h>
#include <hilti/jit/libhilti-jit.h>

using namespace std;

const char* Name = "hiltic";

int  num_output_types = 0;
int  num_input_files = 0;
bool output_hilti = false;
bool output_llvm = false;
bool output_llvm_individually = false;
bool output_bitcode = false;
bool output_prototypes = false;
bool dump_ast = false;
bool use_jit = false;
bool add_stdlibs = false;
bool disable_linker = false;
bool cfg = false;
string output;

static struct option long_options[] = {
    { "ast",     no_argument, 0, 'A' },
    { "bitcode", no_argument, 0, 'b' },
    { "debug",   no_argument, 0, 'd' },
    { "cgdebug", required_argument, 0, 'D' },
    { "help",    no_argument, 0, 'h' },
    { "print",   no_argument, 0, 'p' },
    { "print-always",   no_argument, 0, 'W' },
    { "cfg",   no_argument, 0, 'c' },
    { "prototypes", no_argument, 0, 'P' },
    { "output",  required_argument, 0, 'o' },
    { "version", no_argument, 0, 'v' },
    { "profile", no_argument, 0, 'F' },
    { "jit", no_argument, 0, 'j' },
    { "opt", required_argument, 0, 'O' },
    { "add-stdlibs", no_argument, 0, 's' },
    { "disable-linker", no_argument, 0, 'C' },
    { 0, 0, 0, 0 }
};

void usage()
{
    auto dbglist = hilti::Options().cgDebugLabels();
    auto dbgstr = util::strjoin(dbglist.begin(), dbglist.end(), "/");

    cerr << "Usage: " << Name << " [options] <inputs> [ - <options for JIT main()> ]\n"
            "\n"
            "Input files can be *.hlt, *.ll, *.bc, *.bca, *.so. and *.dylib\n"
            "\n"
            "Options:\n"
            "\n"
            "  -A | --ast            Dump intermediary HILTI ASTs to stderr.\n"
            "  -b | --bitcode        Output LLVM bitcode.\n"
            "  -d | --debug          Debug level. Each time increases level. [Default: 0]\n"
            "  -D | --cgdebug <type> Debug output during code generation; type can be " << dbgstr << ".\n"
            "  -F | --profile        Profile level. Each time increases level. [Default: 0]\n"
            "  -h | --help           Print usage information.\n"
            "  -l | --llvm           Output the final LLVM assembly.\n"
#ifndef HILTIC_NO_JIT
            "  -j | --jit            JIT the final LLVM bitcode to native code and execute main().\n"
#endif
            "  -s | --add-stdlibs    Add standard HILTI runtime libraries (implied with -j).\n"
            "  -L | --llvm-always    Like -l, but don't verify correctness first.\n"
            "  -V | --llvm-first     Like -L, but print each file individually to stdout and don't link.\n"
            "  -o | --output <file>  Specify output file.                    [Default: stdout].\n"
            "  -O | --opt            Optimize generated code.                [Default: off].\n"
            "  -p | --print          Just output all parsed HILTI code again.\n"
            "  -c | --cfg            Add control/data flow information to output of -p.\n"
            "  -P | --prototypes     Generate C prototypes for HILTI module.\n"
            "  -W | --print-always   Like -p, but don't verify correctness first.\n"
            "  -I | --import <dir>   Search library files in <dir>. Can be given multiple times.\n"
            "  -C | --disable-linker Don't run code through the custom HILTI linker; can only be used with one module.\n"
            "  -v | --version        Print version information.\n"
            "\n";
}

void version()
{
    cerr << Name << " v" << hilti::version() << endl;
}

void error(const string& file, const string& msg)
{
    if ( file.size() )
        cerr << util::basename(file) << ": ";

    cerr << msg << endl;
    exit(1);
}

void openOutputStream(ofstream& out, const string& path)
{
    out.open(path, ios::out | ios::trunc);

    if ( ! out.is_open() )
        error(path, "cannot open file for output");
}

llvm::Module* loadLLVM(const string& path)
{
    // This works for both LLVM bitcode and assembly files.
    llvm::SMDiagnostic diag;
    llvm::Module *module = llvm::ParseIRFile(path, diag, llvm::getGlobalContext());

    if ( ! module ) {
        string dummy;
        llvm::raw_string_ostream os(dummy);
        diag.print("", os);

        if ( diag.getLineNo() > 0 )
            error(path, util::fmt("line %d, %s", diag.getLineNo(), diag.getMessage().c_str()));
        else
            error(path, diag.getMessage().c_str());
    }

    return module;
}

llvm::Module* compileHILTI(std::shared_ptr<hilti::CompilerContext> ctx, string path)
{
    auto module = ctx->loadModule(path);

    if ( ! module )
        error(path, "Aborting due to verification error.");

    if ( dump_ast )
        ctx->dump(module, cerr);

    if ( output_hilti ) {
        ofstream out;
        openOutputStream(out, output);

        if ( num_input_files > 1 )
            out << "<<< Begin " << path << endl;

        bool success = ctx->print(module, out, cfg);

        if ( num_input_files > 1 )
            out << ">>> End " << path << endl;

        if ( ! success )
            error(path, "Aborting due to printer error.");

        return nullptr;
    }

    if ( output_prototypes ) {
        ofstream out;
        openOutputStream(out, output);

        ctx->generatePrototypes(module, out);

        return nullptr;
    }

    llvm::Module* llvm_module = ctx->compile(module);

    if ( ! llvm_module )
        error(path, "Aborting due to code generation error.");

    if ( output_llvm_individually ) {
        if ( num_input_files > 1 )
            cout << "<<< Begin " << path << endl;

        ctx->printBitcode(llvm_module, cout);

        if ( num_input_files > 1 )
            cout << ">>> End " << path << endl;

    }

    return llvm_module;
}

bool runJIT(shared_ptr<hilti::CompilerContext> ctx, llvm::Module* module, std::list<string> jitargs)
{
#ifdef HILTIC_NO_JIT
    error(0, "No JIT support compiled into this version of hiltic.");
    return false;
#else

    auto ee =  ctx->jitModule(module);

    if ( ! ee )
        return false;

    hlt_init_jit(ctx, module, ee);

    auto libmain = (int (*)(int , const char**)) ctx->nativeFunction(module, ee, "__libhilti_main");

    if ( ! libmain )
        error(0, "internal error: no __libhilti_main in compiled module");

    // Create argv[] for the main() function we'll call.
    int argc = jitargs.size() + 1;
    const char *argv[argc + 1];
    argv[0] = module->getModuleIdentifier().c_str(); // Make up an argv[0].

    int i = 1;
    for ( auto a : jitargs )
        argv[i++] = a.c_str();

    argv[i] = nullptr;

    (*libmain)(argc, argv);

    return true;
#endif
}

int main(int argc, char** argv)
{
    int num_output_types = 0;

    hilti::Options options;

    while ( true ) {
        int c = getopt_long(argc, argv, "AdD:hjpcFPWbClLsVo:OvI:", long_options, 0);

        if ( c < 0 )
            break;

        switch (c) {
         case 'A':
            dump_ast = true;
            break;

         case 'b':
            output_bitcode = true;
            ++num_output_types;
            break;

         case 'd':
            options.debug = true;
            break;

         case 'D':
            options.cg_debug.insert(optarg);
            break;

         case 'o':
            output = optarg;
            break;

         case 'O':
            options.optimize = true;
            break;

         case 'p':
            output_hilti = true;
            ++num_output_types;
            break;

         case 'c':
            cfg = true;
            break;

         case 'P':
            output_prototypes = true;
            ++num_output_types;
            break;

         case 'W':
            output_hilti = true;
            options.verify = false;
            ++num_output_types;
            break;

         case 'l':
            output_llvm = true;
            ++num_output_types;
            break;

         case 'L':
            output_llvm = true;
            options.verify = false;
            ++num_output_types;
            break;

         case 'V':
            output_llvm_individually = true;
            options.verify = false;
            ++num_output_types;
            break;

         case 'I':
            options.libdirs_hlt.push_back(optarg);
            break;

         case 'F':
            options.profile = true;
            break;

         case 'j':
            use_jit = true;
            add_stdlibs = true;
            ++num_output_types;
            break;

         case 's':
            add_stdlibs = true;
            break;

         case 'C':
            disable_linker = true;
            break;

         case 'v':
            ::version();
            return 0;

         case 'h':
            usage();
            return 0;

         default:
            usage();
            return 1;
        }

    }

    std::list<string> inputs;
    std::list<string> jitargs;

    while ( optind < argc && string("-") != argv[optind] )
        inputs.push_back(argv[optind++]);

    while ( optind < argc - 1 )
        jitargs.push_back(argv[++optind]);

    num_input_files = inputs.size();

    if ( num_input_files == 0 )
        error("", "No input file given.");

    if ( disable_linker && num_input_files != 1 )
        error("", "Cannot use more than one input file when not linking");

    if ( disable_linker && use_jit )
        error("", "Cannot use JIT when not linking");

    if ( num_output_types == 0 )
        error("", "No output type specificied.");

    if ( num_output_types > 1 )
        error("", "Only one type of output can be specificied.");

    if ( ! output.size() ) {
        if ( output_bitcode )
            error("", "No output file given.");

        if ( use_jit )
            output = "<JIT code>";
        else
            output = "/dev/stdout";
    }

    hilti::init();

    std::list<llvm::Module*> modules;
    std::list<string> libs; // Not filled currently.
    path_list bcas;
    path_list dylds;

    auto ctx = std::make_shared<hilti::CompilerContext>(options);

    // Go through input files and prepare LLVM modules.
    for ( auto input : inputs ) {

        llvm::Module* module = 0;

        if ( util::endsWith(input, ".hlt") )
            module = compileHILTI(ctx, input);

        else if ( util::endsWith(input, ".ll") ||
                  util::endsWith(input, ".bc") )
            module = loadLLVM(input);

        else if ( util::endsWith(input, ".bca") )
            bcas.push_back(input);

        else if ( util::endsWith(input, ".so") ||
                  util::endsWith(input, ".dylib") )
            dylds.push_back(input);

        else
            error(input, "Unsupport file type.");

        if ( module )
            modules.push_back(module);
    }

    if ( output_hilti || output_llvm_individually || output_prototypes )
        // Done.
        return 0;

    if ( modules.size() == 0 )
        error("", "Nothing to link.");

    llvm::Module* linked_module = nullptr;

    if ( ! disable_linker ) {
        linked_module = ctx->linkModules(output, modules, libs, bcas, dylds, add_stdlibs, use_jit);

        if ( ! linked_module )
            error(output, "Aborted linking.");
    }

    else
        linked_module = modules.front();

    if ( use_jit ) {
        if ( ! runJIT(ctx, linked_module, jitargs) )
            return 1;
        else
            return 0;
    }

    ofstream out;
    openOutputStream(out, output);

    if ( output_llvm )
        ctx->printBitcode(linked_module, out);

    if ( output_bitcode )
        ctx->writeBitcode(linked_module, out);

    return 0;
}
