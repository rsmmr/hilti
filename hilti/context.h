
#ifndef HILTI_CONTEXT_H
#define HILTI_CONTEXT_H

#include <ast/logger.h>

#include "common.h"
#include "module.h"

namespace llvm {
    class Module;
    class ExecutionEngine;
}

namespace hilti {

namespace passes { class ScopeBuilder; }
namespace jit { class JIT; }

/// A module context that groups a set of modules compiled jointly. This
/// class provides the main top-level interface for parsing, compiling, and
/// linking HILTI modules.
class CompilerContext : public ast::Logger, public std::enable_shared_from_this<CompilerContext>
{
public:
    /// Constructor.
    ///
    /// libdirs: List of directories to search for imports and other library
    /// files. The current directory will always be tried first.
    CompilerContext(const string_list libdirs = string_list());

    /// Destructor.
    ~CompilerContext();

    /// Reads a HILTI module from a file returns the parsed AST. It uses uses
    /// parse() for parsing and, by default, also fully finalizes the module.
    ///
    /// path: The path to the file to parse.
    ///
    /// verify: If false, no correctness verification is done. Only applies
    /// if \a finalize is true.
    ///
    /// finalize: If false, finalize() isn't called.
    ///
    /// Returns: The loaded Module, or null on error. On error, an error
    /// message will have been reported.
    shared_ptr<Module> loadModule(const string& path, bool verify = true, bool finalize = true);

    /// Imports a HILTI module into the context. This is mainly an internal
    /// method; it makes the module available for later ID resolving. It does
    /// not finalize.
    ///
    /// id: The name of the module. Note that this corresponds to the
    /// file-system name, which may differ from the module's declared
    /// identifier.
    ///
    /// verify: If false, no correctness verification is done. Only applies
    /// if \a finalize is true.
    ///
    /// finalize: If false, finalize() isn't called.
    ///
    /// Returns: True on success.
    bool importModule(shared_ptr<ID> id);

    /// Finds the file a module is to be imported from.
    ///
    /// id: The module to import.
    ///
    /// Returns: The absolute pathname of the file, or an empty string if not
    /// found. In the error case, an error message wil have been reported.
    string searchModule(shared_ptr<ID> id);

    /// Returns an alias to a module's scope. An alias is a new scope that
    /// however will reflect any changes later made to the module's original
    /// scope. But the alias' parent scope is maintainded independently so
    /// that it can be chained into another module. If the specified module
    /// hasn't been loaded yet, it will be done so via loadModule().
    ///
    /// id: The module to return an alias scope for.
    ///
    /// Returns: The scope, or null on error.
    shared_ptr<Scope> scopeAlias(shared_ptr<ID> id);

    /// Parses HILTI source code into a module AST. Note that before the AST
    /// can be used further, it needs to go through finalize(). The exception
    /// is printAST(), which always works.
    ///
    /// in: The stream the source code is read from.
    ///
    /// sname: The name of stream/file. Not crucial to give, but will be used
    /// in error messages.
    ///
    /// Returns: The parsed module AST, or null if there were errors.
    shared_ptr<Module> parse(std::istream& in, const std::string& sname = "<input>");

    /// Adds an externally built module to the context.
    void addModule(shared_ptr<Module> module);

    /// Finalizes a built module. This resolves all still unresolved
    /// references.
    ///
    /// module: The module.
    ///
    /// verify: If false, no correctness verification is done. Only applies
    /// if \a finalize is true.
    ///
    /// Returns: True if no error occured.
    bool finalize(shared_ptr<Module> module, bool verify);

    /// Compiles an AST into a LLVM module. This is the main interface to the
    /// code generater. The AST must have passed through finalize(). After
    /// compilation, it needs to be linked with linkModules().
    ///
    /// module: The module to compile.
    ///
    /// debug: The debug level to activate for code generation. The higher,
    /// the more debugging code will be compiled in. Zero disables all
    /// debugging.
    ///
    /// verify: True if the resulting HILTI module should be checked for
    /// correctness. This should normally be on except for debugging.
    ///
    /// profile: If true, profiling instrumentation will be included into the
    /// generated module.
    ///
    /// Returns: The HILTI module, or null if errors are encountered. Passes
    /// ownership to the caller.
    llvm::Module* compile(shared_ptr<Module> module, int debug = 0, bool verify = true, int profile = false);

    /// Renders an AST back into HILTI source code.
    ///
    /// module: The AST to render.
    ///
    /// out: The output stream.
    ///
    /// Returns: True if no errors were encountered.
    bool print(shared_ptr<Module> module, std::ostream& out);

    /// Prints out an LLVM module as IR. This works mostly like LLVM's internal
    /// printing mechanism, yet for modules generated by the HILTI code
    /// generator, it may insert additional comments making it easier to track
    /// where code is coming from.
    ///
    /// module: The LLVM module to print.
    ///
    /// out: The stream to print LLVM IR to.
    bool printBitcode(llvm::Module* module, std::ostream& out);

    /// Generates the C prototype for a HILTI module. This prints a *.h to the
    /// given stream describing the module's interface for host applications.
    ///
    /// ast: The module to generate prototypes for.
    ///
    /// out: The stream to write it out to.
    void generatePrototypes(shared_ptr<Module> module, std::ostream& out);

    /// Writes out ab LLVM module as bitcode. This directly forwards to the
    /// corresponding LLVM functionality.
    ///
    /// module: The LLVM module to print.
    ///
    /// out: The stream to print LLVM bitcode to.
    bool writeBitcode(llvm::Module* module, std::ostream& out);

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
    /// dylds: Additional dynamic libraries to be loaded. The given libraries
    /// will be searched along the usual runtime path by the system linker.
    ///
    /// debug: The debug level to activate for linking. The higher, the more
    /// debugging code will be compiled in. Zero disables all debugging. This
    /// also controls whether the debug or release runtime library will be
    /// linked in with \a add_stdlibs.
    ///
    /// verify: True if the resulting LLVM module should be checked for
    /// correctness. This should normally be on except for debugging.
    ///
    /// add_stdlibs: Link in HILTI's standard runtime libraries.
    ///
    /// add_sharedlibs: Link in system-wide shared libraries that the runtime
    /// will need.
    ///
    /// Returns: The composite LLVM module, or null if errors are encountered.
    llvm::Module* linkModules(string output,
                              std::list<llvm::Module*> modules,
                              path_list paths = {},
                              std::list<string> libs = {},
                              path_list bcas = {},
                              path_list dylds = {},
                              bool debug = true,
                              bool verify = true,
                              bool add_stdlibs = true,
                              bool add_sharedlibs = true);

    /// JITs an LLVM module retuned by linkModules().
    ///
    /// module: The module. The function takes ownership.
    ///
    /// optimize: The optimization level, corresponding to \c -Ox
    ///
    /// Returns: The LLVM execution engine that was used for JITing the
    /// module, or null if there was an error. This passes ownership of the
    /// engine to the caller.
    llvm::ExecutionEngine* jitModule(llvm::Module* module, int optimize);

    /// Returns a pointer to a compiled, native function after a module has
    /// beed JITed. This must only be called after jitModule().
    ///
    /// module: The LLVM module that was passed to jitModule().
    ///
    /// ee: The LLVM execution engine that jitModule() returned.
    ///
    /// function: The name of the function.
    ///
    /// Returns: A pointer to the function (which must be suitably casted),
    /// or null if that function doesn't exist.
    void* nativeFunction(llvm::Module* module, llvm::ExecutionEngine* ee, const string& function);

    struct FunctionMapping {
        const char* name; /// Name of the function.
        void *func;       /// Address of the function.
    };

    /// Installs a table to resolve functions at JIT time that are located
    /// statically inside the main process.
    ///
    /// mappings: An array of name-to-address mappings. The last entry must
    /// be null pointers to mark the end of the array.
    void installFunctionTable(const FunctionMapping* ftable);

    /// Dumps out an AST in (somewhat) readable format for debugging.
    ///
    /// ast: The AST to dump. This can be a partial AST, i.e., it doesn't need
    /// to start with a Module node. It also doesn't need to be finalized.
    ///
    /// out: The stream to write it out to.
    ///
    /// Returns: True if no errors are encountered.
    bool dump(shared_ptr<Node> ast, std::ostream& out);

    /// Enables debugging output during code generation.
    ///
    /// string: A label for the desired information. \a debugStreams()
    /// returns the available list.
    ///
    /// Returns: True if the label does not correspond to a valid debug
    /// stream.
    bool enableDebug(const string& label);

    /// Enables debugging output during code generation.
    ///
    /// labels: A set of labels, one for each of the desired streams. \a
    /// debugStreams() returns the available list.
    ///
    /// Returns: True if the label does not correspond to a valid debug
    /// stream.
    bool enableDebug(std::set<string>& labels);

    /// Returns true if a given debug stream is active.
    ///
    /// string: The label for the desired information. \a debugStreams()
    /// returns the available list.
    bool debugging(const string& label);

    /// Returns the library dirs configured.
    const string_list& libraryPaths() const;

    /// Returns the available debug streams during code generation.
    static std::list<string> debugStreams();

    /// Returns true if the given lavel correspnds to a valid debugging
    /// stream.
    static bool validDebugStream(const string& label);

private:
    /// Finalizes an AST before it can be used for compilation. This checks
    /// for correctness, resolves any unknown bindings, and builds a set of
    /// internal data structures.
    ///
    /// module: The AST to finalize.
    ///
    /// verify: If false, no correctness verification is done.
    ///
    /// Returns: True if no errors were found.
    ///
    bool _finalizeModule(shared_ptr<Module> module, bool verify);

    string_list _libdirs;
    std::set<string> _debug_streams;
    jit::JIT* _jit = nullptr;

    /// We keep global maps of all module nodes and, separately, their
    /// scopes, both indexed by their path. This is for avoiding duplicate
    /// imports, in particular when encountering cycles.
    std::map<string, shared_ptr<Module>> _modules;
    std::map<string, shared_ptr<Scope>> _scopes;
    std::set<string> _loaded;

    std::list<shared_ptr<ID>> _imported;
};

}

#endif
