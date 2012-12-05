
#ifndef BINPAC_CONTEXT_H
#define BINPAC_CONTEXT_H

#include "common.h"
#include "module.h"

namespace hilti { class Module; }
namespace hilti { class CompilerContext; }
namespace llvm  { class Module; }

#include "libhilti/types.h"

namespace binpac {

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
    CompilerContext(const string_list& libdirs = string_list());

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
    /// verify: If false, no correctness verification is done. Only applies if
    /// \a finalize is true.
    ///
    /// finalize: If false, finalize() isn't called.
    ///
    /// Returns: The parsed AST, or null if errors are encountered.
    shared_ptr<Module> load(string path, bool verify = true, bool resolve = true);

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

    /// Finalizes an AST before it can be used for compilation. This checks
    /// for correctness, resolves any unknown bindings, and builds a set of
    /// internal data structures.
    ///
    /// module: The AST to finalize.
    ///
    /// libdirs: A list of paths to search for any libraries that might be
    /// needed.
    ///
    /// verify: If false, no correctness verification is done.
    ///
    /// Returns: True if no errors were found.
    bool finalize(shared_ptr<Module> module, bool verify = true);

    /// Compiles an AST into a HILTI module. This is the main interface to the
    /// code generater. The AST must have passed through finalize(). After
    /// compilation, it needs to be linked with linkModules().
    ///
    /// module: The module to compile.
    ///
    /// debug: The debug level to activate for code generation. The higher, the
    /// more debugging code will be compiled in. Zero disables all debugging.
    ///
    /// verify: True if the resulting HILTI module should be checked for
    /// correctness. This should normally be on except for debugging.
    ///
    /// Returns: The HILTI module, or null if errors are encountered. Passes
    /// ownership to the caller.
    shared_ptr<hilti::Module> compile(shared_ptr<Module> module, int debug = 0, bool verify = true);

    /// Links a set of compiled BinPAC++ modules into a single LLVM module.
    /// All modules produced by compileModule() must be linked (and all
    /// together that will run as one executable). A module must not be
    /// linked more than once.
    ///
    /// output: The name of the output module. This is mainly for
    /// informational purposes.
    ///
    /// modules: All the HILTI modules that should be linked together. In
    /// addition to modules generate by compileModule(), this can include
    /// further ones created in other ways.
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
    /// debug: The debug level to activate for linking. The higher, the more
    /// debugging code will be compiled in. Zero disables all debugging. This
    /// also controls whether the debug or release runtime library will be
    /// linked in with \a add_stdlibs.
    ///
    /// verify: True if the resulting LLVM module should be checked for
    /// correctness. This should normally be on except for debugging.
    ///
    /// profile: True for adding profiling instrumentation at the HILTI level.
    ///
    /// add_stdlibs: Link in BinPAC++'s standard runtime libraries (and HILTI
    /// standard libraries as well.
    ///
    /// add_sharedlibs: Link in system-wide shared libraries that the runtime
    /// will need.
    ///
    /// Returns: The composite LLVM module, or null if errors are encountered.
    llvm::Module* linkModules(string output, std::list<shared_ptr<hilti::Module>> modules,
                              path_list paths = path_list(), std::list<string> libs = std::list<string>(),
                              path_list bcas = path_list(), path_list dylds = path_list(),
                              bool debug = false, bool verify = true, bool profile = false, bool add_stdlibs = true, bool add_sharedlibs = true);

    /// Links a set of compiled BinPAC++ modules into a single LLVM module.
    /// All modules produced by compileModule() must be linked (and all
    /// together that will run as one executable). A module must not be
    /// linked more than once. This is reduced version of \c linkModules that
    /// offers less options; it sets all other options to their default.
    ///
    /// output: The name of the output module. This is mainly for
    /// informational purposes.
    ///
    /// modules: All the HILTI modules that should be linked together. In
    /// addition to modules generate by compileModule(), this can include
    /// further ones created in other ways.
    ///
    /// debug: The debug level to activate for linking. The higher, the more
    /// debugging code will be compiled in. Zero disables all debugging. This
    /// also controls whether the debug or release runtime library will be
    /// linked in with \a add_stdlibs.
    ///
    /// profile: True for adding profiling instrumentation at the HILTI level.
    ///
    /// Returns: The composite LLVM module, or null if errors are encountered.
    llvm::Module* linkModules(string output, std::list<shared_ptr<hilti::Module>> modules,
                              bool debug = false, bool profile = false);

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

    /// Enables additional debugging output during code generation.
    ///
    /// string: A label for the desired information. \a debugStreams()
    /// returns the available list.
    ///
    /// Returns: True if the label does not correspond to a valid debug
    /// stream.
    bool enableDebug(const string& label);

    /// Enables additional debugging output during code generation.
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

    /// Returns the HILTI context for building the HILTI modules.
    shared_ptr<hilti::CompilerContext> hiltiContext() const;

    /// Returns the available debug streams during code generation.
    static std::list<string> debugStreams();

    /// Returns true if the given lavel correspnds to a valid debugging
    /// stream.
    static bool validDebugStream(const string& label);

private:
    string_list _libdirs;
    std::set<string> _debug_streams;
    shared_ptr<hilti::CompilerContext> _hilti_context;

    /// We keep a global map of all module nodes we have instantiated so far,
    /// indexed by their path. This is for avoid duplicate imports, in
    /// particular when encountering cycles.
    std::map<string, shared_ptr<Module>> _modules;
};

}

#endif
