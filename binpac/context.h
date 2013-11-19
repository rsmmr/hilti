
#ifndef BINPAC_CONTEXT_H
#define BINPAC_CONTEXT_H

#include "common.h"
#include "module.h"
#include "util/file-cache.h"

namespace hilti { class Module; class Type; }
namespace hilti { class CompilerContext; }
namespace llvm  { class Module; }

#include "libhilti/types.h"

namespace binpac {

class Options;

/// A module context that groups a set of modules compiled jointly. This
/// class provides the main top-level interface for parsing and compiling
/// BinPAC++ modules.
class CompilerContext : public Logger, public std::enable_shared_from_this<CompilerContext>
{
public:
    /// Constructor.
    ///
    /// libdirs: List of directories to search for relative paths. The current
    /// directly will be tried first.
    CompilerContext(const Options& options);

    /// Returns the set of options in effect.
    const Options& options() const;

    /// Reads a BinPAC++ source file and returns the parsed AST. The function
    /// searches the file in the paths given and reads it in. It then uses uses
    /// parseStream() for parsing. By default, the module is fully finalized.
    ///
    /// path: The relative or absolute path to the source to load.
    ///
    /// import: True if this load is triggered recurisvely by an import
    /// statement. In that case the returned module may not be fully resolved.
    /// Should be used ony for internal purposes.
    ///
    /// finalize: If false, finalize() isn't called.
    ///
    /// Returns: The parsed AST, or null if errors are encountered.
    shared_ptr<Module> load(string path, bool finalize = true);

    /// Parses BinPAC++ source code into a module AST. Note that before the
    /// AST can be used further, it needs to go through finalize(). The
    /// exception is printAST(), which always works.
    ///
    /// in: The stream the source code is read from.
    ///
    /// sname: The name of stream/file. Not crucial to give, but will be used in
    /// error messages.
    ///
    /// Returns: The parsed module AST, or null if there were errors.
    shared_ptr<Module> parse(std::istream& in, const std::string& sname = "<input>");

    /// Parses a single BinPAC++ expression into a corresponding AST.
    ///
    /// expr: The expression to parse.
    ///
    /// Returns: The parsed expression, or null if there were errors.
    shared_ptr<Expression> parseExpression(const std::string& expr);

    /// Finalizes an AST before it can be used for compilation. This checks
    /// for correctness, resolves any unknown bindings, and builds a set of
    /// internal data structures.
    ///
    /// module: The AST to finalize.
    ///
    /// Returns: True if no errors were found.
    bool finalize(shared_ptr<Module> module);

    /// Partially finalizes an AST. This attempts to resolved as many
    /// unknown bindings as possible, but doesn't further validate the
    /// result.
    ///
    /// node: The root node of the branch to finalize.
    ///
    /// Returns: True if the partial finalizing proceeded as expected.
    bool partialFinalize(shared_ptr<Module> module);

    /// Compiles an AST into an LLVM module. This is the main interface to
    /// the code generater. The AST must have passed through finalize().
    /// After compilation, it needs to be linked with linkModules().
    ///
    /// module: The module to compile.
    ///
    /// hilti_module: If given, a pointer to the intermediary HILTI module
    /// will be stored in there.
    ///
    /// hilti_only: If true, the method compiles only into HILTI but skips
    /// the final llvm step.
    ///
    /// Returns: The LLVM module, or null if errors are encountered. Passes
    /// ownership to the caller. Also returns null if *hilti_only* is true.
    llvm::Module* compile(shared_ptr<Module> module, shared_ptr<hilti::Module>* hilti_module = nullptr, bool hilti_only = false);

    /// Compiles a BinPAC++ source file into an LLVM module. Internally, this
    /// is a combination of load() and compile(), with additional caching if
    /// enabled. After compilation, the module needs to be linked with
    /// linkModules().
    ///
    /// path: The relative or absolute path to the source to load.
    ///
    /// hilti_module: If given, a pointer to the intermediary HILTI module
    /// will be stored in there.
    ///
    /// Returns: The HILTI module, or null if errors are encountered. Passes
    /// ownership to the caller.
    llvm::Module* compile(const string& path, shared_ptr<hilti::Module>* hilti_module = nullptr);

    /// Links a set of compiled BinPAC++ modules into a single LLVM module.
    /// All modules produced by compileModule() must be linked (and all
    /// together that will run as one executable). A module must not be
    /// linked more than once.
    ///
    /// output: The name of the output module. This is mainly for
    /// informational purposes.
    ///
    /// modules: All the LLVM modules that should be linked together. In
    /// addition to modules generate by compileModule(), this can include
    /// further ones created in other ways. The method takes ownership.
    ///
    /// paths: Paths to search for any library files.
    ///
    /// libs: Additional native libraries to link in (optional).
    ///
    /// bca: Additional LLVM bitcode archives to link in (*.bca; optional).
    ///
    /// dylds: Additional dynamic libraries to be loaded. The given libraries
    /// will be searched along the usual runtime path by the system linker
    /// (optional).
    ///
    /// add_stdlibs: Link in BinPAC++'s standard runtime libraries (and HILTI
    /// standard libraries as well.
    ///
    /// add_sharedlibs: Link in system-wide shared libraries that the runtime
    /// will need.
    ///
    /// Returns: The composite LLVM module, or null if errors are
    /// encountered. Passes ownership to caller.
    llvm::Module* linkModules(string output, std::list<llvm::Module*> modules,
                              std::list<string> libs,
                              path_list bcas = path_list(), path_list dylds = path_list(),
                              bool add_stdlibs = true, bool add_sharedlibs = false);

    /// Links a set of compiled BinPAC++ modules into a single LLVM module.
    /// All modules produced by compileModule() must be linked (and all
    /// together that will run as one executable). A module must not be
    /// linked more than once. This is reduced version of \c linkModules that
    /// offers less options; it sets all other options to their defaults.
    ///
    /// output: The name of the output module. This is mainly for
    /// informational purposes.
    ///
    /// modules: All the LLVM modules that should be linked together. In
    /// addition to modules generate by compileModule(), this can include
    /// further ones created in other ways. The method takes ownership.
    ///
    /// Returns: The composite LLVM module, or null if errors are
    /// encountered. Passes ownership to caller.
    llvm::Module* linkModules(string output, std::list<llvm::Module*> modules);

    /// Returns the HILTI type that the code generator will use for a given
    /// BinPAC++ type.
    ///
    /// type: The BinPAC++ type.
    ///
    /// Returns: The corresponding HILTI type.
    shared_ptr<::hilti::Type> hiltiType(shared_ptr<binpac::Type> type);

    /// Renders an AST back into BinPAC++ source code.
    ///
    /// module: The AST to render.
    ///
    /// out: The output stream.
    ///
    /// Returns: True if no errors were encountered.
    bool print(shared_ptr<Module> module, std::ostream& out);

    /// Dumps out an AST in (somewhat) readable for debugging.
    ///
    /// ast: The AST to dump. This can be a partial AST, i.e., it doesn't need
    /// to start with a Module node. It also doesn't need to be finalized.
    ///
    /// out: The stream to write it out to.
    ///
    /// Returns: True if no errors are encountered.
    bool dump(shared_ptr<Node> ast, std::ostream& out);

    /// Returns the HILTI context for building the HILTI modules.
    shared_ptr<hilti::CompilerContext> hiltiContext() const;

    /// Augments the cache key with values suitable to check if a module (or
    /// any of its dependencies) has changed.
    ///
    /// \todo Dependencies aren't tracked yet.
    ///
    /// module: The module to update the key for.
    ///
    /// key: The key to update.
    void toCacheKey(shared_ptr<Module> module, ::util::cache::FileCache::Key* key);

    /// Returns a list of path names that this module depends on.
    ///
    /// \todo This is actually not yet implemented and always returns an
    /// empty list currently.
    std::list<string> dependencies(shared_ptr<Module> module);

private:
    shared_ptr<Options> _options;
    shared_ptr<hilti::CompilerContext> _hilti_context;

    // Backend for both finalize() and partialFinalize().
    bool finalize(shared_ptr<Module> module, bool verify);

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

    /// We keep a global map of all module nodes we have instantiated so far,
    /// indexed by their path. This is for avoid duplicate imports, in
    /// particular when encountering cycles.
    std::map<string, shared_ptr<Module>> _modules;
};

}

#endif
