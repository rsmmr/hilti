///
/// Top-level include file.
///

#ifndef HILTI_H
#define HILTI_H

#include <fstream>

#include "common.h"
#include "module.h"
#include "constant.h"
#include "declaration.h"
#include "expression.h"
#include "function.h"
#include "id.h"
#include "instruction.h"
#include "pass.h"
#include "statement.h"
#include "type.h"
#include "variable.h"

#include "passes/passes.h"
#include "parser/driver.h"
#include "codegen/codegen.h"
#include "codegen/linker.h"
#include "codegen/asm-annotater.h"

// Must come last.
#include "visitor-interface.h"

namespace llvm { class Module; }

namespace hilti {

/// Parses HILTI source code into a module AST. Any errors are written out to
/// stderr. Note that before the AST can be used further, it needs to go
/// through resolveAST(). The exception is printAST(), which always works.
///
/// in: The stream the source code is read from.
///
/// sname: The name of stream/file. Not crucial to give, but will be used in
/// error messages.
///
/// Returns: The parsed module AST, or null if there were errors.
inline shared_ptr<Module> parseStream(std::istream& in, const std::string& sname = "<input>")
{
    hilti_parser::Driver driver;
    return driver.parse(in, sname);
}

/// Renders an AST back into HILTI source code.
///
/// module: The AST to render.
///
/// out: The output stream.
///
/// Returns: True if no errors were encountered.
inline bool printAST(shared_ptr<Module> module, std::ostream& out)
{
    passes::Printer printer(out);
    return printer.run(module);
}

/// Resolves unknown bindings in an AST. An AST must be processed with this
/// function after it has been parsed with parseStream() and before it can be
/// passed on to validateAST() and compileModule(). The function modifies the
/// AST in place. If it encounters any errors, message is written to stderr.
///
/// module: The AST to resolve.
///
/// libdirs: A list of paths to search for any libraries that might be
/// needed.
///
/// Returns: True if no errors were found.
extern bool resolveAST(shared_ptr<Module> module, const path_list& libdirs);

/// Verifyies the semantic correctness of an AST. This function should be
/// called before code generation, and must be called only after resolveAST
/// (as otherwise it will almost certainly fail). If it encounters any
/// errors, message will be written to stderr.
///
/// Returns: True if no errors are found.
inline bool validateAST(shared_ptr<Module> module)
{
    passes::Validator validator;
    return validator.run(module);
}

/// Reads a HILTI source file and returns the parsed AST. The function
/// searches the file in the paths given and reads it in. It then uses
/// uses parseStream() for parsing.
///
/// path: The relative or absolute path to the source to load.
///
/// libdirs: List of directories to search for relative paths. The current
/// directly will be tried first.
///
/// Returns: The parsed AST, or null if errors are encountered.
inline shared_ptr<Module> loadModule(string path, const path_list& libdirs)
{
    if ( ! util::endsWith(path, ".hlt") )
        path += ".hlt";

    string full_path = util::findInPaths(path, libdirs);

    if ( full_path.size() == 0 ) {
        std::cerr << "cannot find " + path << std::endl;
        return nullptr;
    }

    shared_ptr<Module> module = Module::getExistingModule(full_path);

    if ( module )
        return module;

    std::ifstream in(full_path);

    if ( in.fail() ) {
        std::cerr << "cannot open " + full_path << " for reading" << std::endl;
        return nullptr;
    }

    module = hilti::parseStream(in, full_path);

    if ( ! module )
        return nullptr;

    if ( ! resolveAST(module, libdirs) )
        return nullptr;

    if ( ! validateAST(module) )
        return nullptr;

    return module;
}

/// Reads a HILTI module from its source file and returns the parsed AST. The
/// function searches the module file in the paths given and reads it in. It
/// then uses parseStream() for parsing.
///
/// id: An ID with the name of a HILTI module to load.
///
/// libdirs: List of directories to search for relative paths. The current
/// directly will be tried first.
///
/// Returns: The parsed AST, or null if errors are encountered.
inline shared_ptr<Module> loadModule(shared_ptr<ID> id, const path_list& libdirs)
{
    return loadModule(id->name(), libdirs);
}

/// Dumps out an AST in (somewhat) readable for for debugging.
///
/// ast: The AST to dump. This can be a partial AST, i.e., it doesn't need to
/// start with a Module node.
///
/// out: The stream to write it out to.
///
/// Returns: True if no errors are encountered.
inline bool dumpAST(shared_ptr<Node> ast, std::ostream& out)
{
    ast->dump(out);
    return true;
}

/// Compiles an AST into an LLVM module. This is the main interface to the
/// code generater. The AST must have passed through resolveAST() and should
/// have checked with validateAST(). After compilation, it needs to be linked
/// with linkModules().
///
/// module: The module to compile.
///
/// libdirs: List of directories to search for libarary files.
///
/// verify: True if the resulting LLVM module should be checked for
/// correctness. This should normally be on except for debugging.
///
/// debug: The debug level to activate for code generation. The higher, the
/// more debugging code will be compiled in. Zero disables all debugging.
///
/// profiling: The profiling level to activate for code generation. The
/// higher, the more profiling code will be compiled in. Zero disables all
/// profiling.
///
/// debug_cg: The debugging level for the code generator itself. The higher,
/// the more debugging output will be generated during code generation. Zero
/// disables all debugging output.
///
/// Returns: The LLVM module, or null if errors are encountered. Passes
/// ownership to the caller.
inline llvm::Module* compileModule(shared_ptr<Module> module, const path_list& libdirs, bool verify, int debug, int profile, int debug_cg)
{
    codegen::CodeGen cg(libdirs, debug_cg);
    return cg.generateLLVM(module, verify, debug, profile);
}

/// Links a set of modules with HILTI's custom linker. All modules produced
/// by compileModule() must be linked (and all together that will run as one
/// executable). A module must not be linked more than once.
///
/// output: The name of the output module. This is mainly for informational purposes.
///
/// modules: All the LLVM modules that should be linked together. In addition
/// to modules generate by compileModule(), this can include further ones created in other ways.
///
/// paths: Paths to search for any library files.
///
/// libs: Additional native libraries to link in (optional).
///
/// bca: Additional LLVM bitcode archives to link in (*.bca; optional).
///
/// verify: True if the resulting LLVM module should be checked for
/// correctness. This should normally be on except for debugging.
///
/// debug_cg: The debugging level for the linker. The higher, the more
/// debugging output will be generated during linking. Zero disables all
/// debugging output.
///
/// Returns: The composite LLVM module, or null if errors are encountered.
inline llvm::Module* linkModules(string output, const std::list<llvm::Module*>& modules, const path_list& paths, const std::list<string>& libs, const path_list& bcas, bool verify, int debug_cg)
{
    codegen::Linker linker(libs);

    for ( auto l : libs )
        linker.addNativeLibrary(l);

    for ( auto b : bcas )
        linker.addBitcodeArchive(b);

    return linker.link(output, modules, verify, debug_cg);
}


/// Prints out an LLVM module as IR. This works mostly like LLVM's internal
/// printing mechanism, yet for modules generated by the HILTI code
/// generator, it may insert additional comments making it easier to track
/// where code is coming from.
///
/// out: The stream to print LLVM IR to.
///
/// module: The LLVM module to print.
inline void printAssembly(llvm::raw_ostream &out, llvm::Module* module)
{
    hilti::codegen::AssemblyAnnotationWriter annotator;
    module->print(out, &annotator);
}

}

#endif
