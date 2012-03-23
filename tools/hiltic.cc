
#define __STDC_LIMIT_MACROS     // LLVM needs these.
#define __STDC_CONSTANT_MACROS

#include <getopt.h>
#include <iostream>
#include <fstream>

#include <llvm/Module.h>

#include <hilti.h>

using namespace std;

const char* Name = "hiltic";
const char* Version = "0.2";

int  debug = 0;
int  debug_cg = 0;
int  num_output_types = 0;
int  num_input_files = 0;
bool output_hilti = false;
bool output_llvm = false;
bool output_llvm_individually = false;
bool output_bitcode = false;
bool dump_ast = false;
bool verify = true;


string output;
path_list paths;

static struct option long_options[] = {
    { "ast",     no_argument, 0, 'A' },
    { "bitcode", no_argument, 0, 'b' },
    { "debug",   no_argument, 0, 'd' },
    { "help",    no_argument, 0, 'h' },
    { "print",   no_argument, 0, 'p' },
    { "output",  required_argument, 0, 'o' },
    { "version", no_argument, 0, 'v' },
    { 0, 0, 0, 0 }
};

void usage()
{
    cerr << "Usage: " << Name << " [options] <inputs> \n"
            "\n"
            "Input files can be *.hlt, *.ll, *.bc, and *.bca\n"
            "\n"
            "Options:\n"
            "\n"
            "  -A | --ast            Dump intermediary HILTI ASTs to stderr.\n"
            "  -b | --bitcode        Output LLVM bitcode.\n"
            "  -d | --debug          Debug level. Each time increases level. [Default: 0]\n"
            "  -D | --debug-codegen  Code generation debug level. Each time increases level. [Default: 0]\n"
            "  -h | --help           Print usage information.\n"
            "  -l | --llvm           Output the final LLVM assembly.\n"
            "  -L | --llvm-always    Like -l, but don't verify correctness first.\n"
            "  -V | --llvm-first     Like -L, but print each file individually to stdout and don't link.\n"
            "  -o | --output <file>  Specify output file.                    [Default: stdout].\n"
            "  -p | --print          Just output all parsed HILTI code again.\n"
            "  -P | --print-always   Like -p, but don't verify correctness first.\n"
            "  -I | --import <dir>   Search library files in <dir>. Can be given multiple times.\n"
            "  -v | --version        Print version information.\n"
            "\n";
}

void version()
{
    cerr << Name << " v" << Version << endl;
}

void error(const string& file, const string& msg)
{
    if ( file.size() )
        cerr << util::basename(file) << ": ";

    cerr << msg << endl;
    exit(1);
}

void dumpAST(const string& path, shared_ptr<Module> ast, const char* tag)
{
    if ( ! dump_ast )
        return;

    cerr << "==== " << path << ": AST dump / " << tag << endl;
    hilti::dumpAST(ast, cerr);
    cerr << "==== " << endl << endl;
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

llvm::Module* compileHILTI(string path)
{
    ifstream in(path);
    shared_ptr<hilti::Module> module = hilti::parseStream(in, path);

    if ( ! module )
        error(path, "Aborting due to parse error.");

    dumpAST(path, module, "Before resolver");

    if ( ! hilti::resolveAST(module, paths) )
        error(path, "Aborting due to resolver error.");

    dumpAST(path, module, "After resolver");

    if ( verify && ! hilti::validateAST(module) )
        error(path, "Aborting due to verification error.");

    if ( output_hilti ) {
        ofstream out;
        openOutputStream(out, output);

        if ( num_input_files > 1 )
            out << "<<< Begin " << path << endl;

        bool success = hilti::printAST(module, out);

        if ( num_input_files > 1 )
            out << ">>> End " << path << endl;

        if ( ! success )
            error(path, "Aborting due to printer error.");
    }

    llvm::Module* llvm_module = hilti::compileModule(module, paths, verify, debug, 0, debug_cg);

    if ( ! llvm_module )
        error(path, "Aborting due to code generation error.");

    if ( output_llvm_individually ) {
        llvm::raw_os_ostream llvm_out(cout);

        if ( num_input_files > 1 )
            cout << "<<< Begin " << path << endl;

        hilti::printAssembly(llvm_out, llvm_module);

        if ( num_input_files > 1 )
            cout << ">>> End " << path << endl;

    }

    return llvm_module;
}

int main(int argc, char** argv)
{
    int num_output_types = 0;

    while ( true ) {
        int c = getopt_long(argc, argv, "AdDhpPblLVo:vI:", long_options, 0);

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
            ++debug;
            break;

         case 'D':
            ++debug_cg;
            break;

         case 'o':
            output = optarg;
            break;

         case 'p':
            output_hilti = true;
            ++num_output_types;
            break;

         case 'P':
            output_hilti = true;
            verify = false;
            ++num_output_types;
            break;

         case 'l':
            output_llvm = true;
            ++num_output_types;
            break;

         case 'L':
            output_llvm = true;
            verify = false;
            ++num_output_types;
            break;

         case 'V':
            output_llvm_individually = true;
            verify = false;
            ++num_output_types;
            break;

         case 'I':
            paths.push_back(optarg);
            break;

         case 'v':
            version();
            return 0;

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

    if ( num_output_types == 0 )
        error("", "No output type specificied.");

    if ( num_output_types > 1 )
        error("", "Only one type of output can be specificied.");

    if ( ! output.size() ) {
        if ( output_bitcode )
            error("", "No output file given.");

        output = "/dev/stdout";
    }

    std::list<string> inputs;

    while ( optind < argc )
        inputs.push_back(argv[optind++]);

    num_input_files = inputs.size();

    std::list<llvm::Module*> modules;
    std::list<string> libs; // Not filled currently.
    path_list bcas;

    // Go through input files and prepare LLVM modules.
    for ( auto input : inputs ) {

        llvm::Module* module = nullptr;

        if ( util::endsWith(input, ".hlt") )
            module = compileHILTI(input);

        else if ( util::endsWith(input, ".ll") ||
                  util::endsWith(input, ".bc") )
            module = loadLLVM(input);

        else if ( util::endsWith(input, ".bca") )
            bcas.push_back(input);

        else
            error(input, "Unsupport file type.");

        if ( module )
            modules.push_back(module);
    }

    if ( output_hilti || output_llvm_individually )
        // Done.
        return 0;

    if ( modules.size() == 0 )
        error("", "Nothing to link.");

    auto linked_module = hilti::linkModules(output, modules, paths, libs, bcas, verify, debug_cg);

    if ( ! linked_module )
        error(output, "Aborted linking.");

    ofstream out;
    openOutputStream(out, output);
    llvm::raw_os_ostream llvm_out(out);

    if ( output_llvm )
        hilti::printAssembly(llvm_out, linked_module);

    if ( output_bitcode )
        llvm::WriteBitcodeToFile(linked_module, llvm_out);

    return 0;
}
