
#ifndef HILTI_CONTEXT_H
#define HILTI_CONTEXT_H

#include <ast/logger.h>

#include "common.h"
#include "module.h"
#include "util/file-cache.h"

namespace llvm {
    class Module;
    class ExecutionEngine;
}

namespace hilti {

namespace passes { class ScopeBuilder; }
namespace jit { class JIT; }

class Options;

/// A module context that groups a set of modules compiled jointly. This
/// class provides the main top-level interface for parsing, compiling, and
/// linking HILTI modules.
class CompilerContext : public ast::Logger, public std::enable_shared_from_this<CompilerContext>
{
public:
    /// Constructor.
    CompilerContext(const Options& options);

    /// Destructor.
    ~CompilerContext();

    /// Returns the set of options in effect.
    const Options& options() const;

    /// Updates the current set of options.
    void setOptions(const Options& options);

    /// Reads a HILTI module from a file returns the parsed AST. It uses uses
    /// parse() for parsing and, by default, also fully finalizes the module.
    ///
    /// path: The path to the file to parse.
    ///
    /// finalize: If false, finalize() isn't called.
    ///
    /// Returns: The loaded Module, or null on error. On error, an error
    /// message will have been reported.
    shared_ptr<Module> loadModule(const string& path, bool finalize = true);

    /// Imports a HILTI module into the context. This is mainly an internal
    /// method; it makes the module available for later ID resolving. It does
    /// not finalize.
    ///
    /// id: The name of the module. Note that this corresponds to the
    /// file-system name, which may differ from the module's declared
    /// identifier.
    ///
    /// path_out: If given, the path to the imported module will be stored in
    /// there.
    ///
    /// Returns: True on success.
    bool importModule(shared_ptr<ID> id, string* path_out = nullptr);

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
    /// Returns: True if no error occured.
    bool finalize(shared_ptr<Module> module);

    /// Compiles an AST into a LLVM module. This is the main interface to the
    /// code generater. The AST must have passed through finalize(). After
    /// compilation, it needs to be linked with linkModules(). If caching is
    /// enabled, this method may return a previously cached copy.
    ///
    /// module: The module to compile.
    ///
    /// Returns: The HILTI module, or null if errors are encountered. Passes
    /// ownership to the caller.
    llvm::Module* compile(shared_ptr<Module> module);

    /// Compiles an AST into a LLVM module. This is a variant of compile()
    /// that takes a prepopulated cache key under which the compiled module
    /// will be stored if caching is enabled. However, this version will
    /// never return a previously cached version, it doesn't check whether
    /// the key already exists. After compilation, it needs to be linked with
    /// linkModules().
    ///
    /// module: The module to compile.
    ///
    /// key: The cache key to store the compiled module under.
    ///
    /// Returns: The HILTI module, or null if errors are encountered. Passes
    /// ownership to the caller.
    llvm::Module* compile(shared_ptr<Module> module, const ::util::cache::FileCache::Key& key);

    /// Optimizes an LLVM module according to the CompilerContext's options
    /// (including not all if the options don't request optimization). Note
    /// that you don't need this if calling jitModule() later; the latter
    /// will optimize automatically.
    ///
    /// module: The module to optimize.
    ///
    /// is_linked: True if this is the final linked module, false if it's an
    /// individual module that will later be linked.
    ///
    /// Returns: True if successful.
    bool optimize(llvm::Module* module, bool is_linked);

    /// Renders an AST back into HILTI source code.
    ///
    /// module: The AST to render.
    ///
    /// out: The output stream.
    ///
    /// cfg: If true, include control/data flow information output.
    ///
    /// Returns: True if no errors were encountered.
    bool print(shared_ptr<Module> module, std::ostream& out, bool cfg = false);

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
    /// libs: Additional native libraries to link in (optional).
    ///
    /// bca: Additional LLVM bitcode archives to link in (*.bca; optional).
    ///
    /// dylds: Additional dynamic libraries to be loaded. The given libraries
    /// will be searched along the usual runtime path by the system linker.
    ///
    /// add_stdlibs: Link in HILTI's standard runtime libraries.
    ///
    /// add_sharedlibs: Link in system-wide shared libraries that the runtime
    /// will need.
    ///
    /// Returns: The composite LLVM module, or null if errors are encountered.
    llvm::Module* linkModules(string output,
                              std::list<llvm::Module*> modules,
                              std::list<string> libs = {},
                              path_list bcas = {},
                              path_list dylds = {},
                              bool add_stdlibs = true,
                              bool add_sharedlibs = false);

    /// JITs an LLVM module retuned by linkModules(). This also optimizes the
    /// code according to the current set of options.
    ///
    /// module: The module. The function takes ownership.
    ///
    /// Returns: The LLVM execution engine that was used for JITing the
    /// module, or null if there was an error. This passes ownership of the
    /// engine to the caller.
    llvm::ExecutionEngine* jitModule(llvm::Module* module);

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
    void installJITFunctionTable(const FunctionMapping* ftable);

    /// Looks up a function installed in the JIT function table.
    ///
    /// name: The function name to look up.
    ///
    /// Returns: A pointer to the function, or null if there's no function
    /// under that name.
    void* lookupJITFunctionInTable(const std::string& name);

    /// Returns the file cache the context is using, or null if none.
    shared_ptr<util::cache::FileCache> fileCache() const;

    /// Dumps out an AST in (somewhat) readable format for debugging.
    ///
    /// ast: The AST to dump. This can be a partial AST, i.e., it doesn't need
    /// to start with a Module node. It also doesn't need to be finalized.
    ///
    /// out: The stream to write it out to.
    ///
    /// Returns: True if no errors are encountered.
    bool dump(shared_ptr<Node> ast, std::ostream& out);

    /// Check if we have cached LLVM modules associated with a cache key.
    ///
    /// The method is a no-op if the context doesn't use caching.
    ///
    /// key: The key to use.
    ///
    /// Returns: The cached modules if available, or an empty list if not.
    /// The latter will always be the case if the context is not using
    /// caching.
    std::list<llvm::Module*> checkCache(const ::util::cache::FileCache::Key& key);

    /// Stores/Updates cached versions for a set of LLVM modules.
    ///
    /// key: The key to associate with *all* the modules.
    ///
    /// module: THe modules to cache under \a key.
    void updateCache(const ::util::cache::FileCache::Key& key, std::list<llvm::Module*> module);

    /// Stores/updates cached version of a single LLVM module.
    ///
    /// key: The key to associate with the module.
    ///
    /// module: THe module to cache under \a key.
    void updateCache(const ::util::cache::FileCache::Key& key, llvm::Module* module);

    /// Augments the cache key with values suitable to check if a HILTI
    /// module (or any of its dependencies) has changed.
    ///
    /// \todo Dependencies aren't tracked yet.
    ///
    /// module: The module to update the key for.
    ///
    /// key: The key to update.
    void toCacheKey(shared_ptr<Module> module, ::util::cache::FileCache::Key* key);

    /// Augments the cache key with values suitable to check if an LLVM
    /// module has changed.
    ///
    /// module: The module to update the key for.
    ///
    /// key: The key to update.
    void toCacheKey(const llvm::Module* module, ::util::cache::FileCache::Key* key);

    /// Returns a list of path names that this module depends on.
    ///
    /// \todo This is actually not yet implemented and always returns an
    /// empty list currently.
    std::list<string> dependencies(shared_ptr<Module> module);

    /// Returns the name of an LLVM module. This first looks for
    /// corresponding meta-data that the code generator inserts and returns
    /// that if found, and the standard LLVM module name otherwise. The meta
    /// data is helpful because LLVM's bitcode represenation doesn't preserve
    /// a module's name.
    static std::string llvmGetModuleIdentifier(llvm::Module* module);

private:
    /// Finalizes an AST before it can be used for compilation. This checks
    /// for correctness, resolves any unknown bindings, and builds a set of
    /// internal data structures.
    ///
    /// module: The AST to finalize.
    ///
    /// Returns: True if no errors were found.
    ///
    bool _finalizeModule(shared_ptr<Module> module);

    /// If debugging stream cg-passes is set, log the beginning of a pass. If timing is requested,
    ///
    /// module: The current module.
    ///
    /// pass: The pass. All we need from it is the name, so this is actually
    /// more generic
    ///
    void _beginPass(shared_ptr<Module> module, const ast::Logger& pass);
    void _beginPass(shared_ptr<Module> module, const string& name);
    void _beginPass(const string& module, const string& name);
    void _beginPass(const string& module, const ast::Logger& pass);

    /// If debugging stream cg-passes is set, log the end of the most recent
    /// pass.
    void _endPass();

    shared_ptr<Options> _options;

    jit::JIT* _jit = nullptr;
    shared_ptr<util::cache::FileCache> _cache;

    /// We keep global maps of all module nodes and, separately, their
    /// scopes, both indexed by their path. This is for avoiding duplicate
    /// imports, in particular when encountering cycles.
    std::map<string, shared_ptr<Module>> _modules;

    /// TODO: binpac context needs this, should be doing it differently.
public:
    // Tracking passes.
    struct PassInfo {
        string module;
        double time;
        string name;
    };

    typedef std::list<PassInfo> pass_list;

private:
    pass_list _passes;

public:
    pass_list& passes() { return _passes; }
};

}

#endif
