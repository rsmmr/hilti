
#ifndef BINPAC_CONTEXT_H
#define BINPAC_CONTEXT_H

#include "common.h"
#include "module.h"

namespace hilti { class Module; }

namespace binpac {

/// A module context that groups a set of modules compiled jointly. This
/// class provides the main top-level interface for parsing and compiling
/// BinPAC++ modules.
class CompilerContext : public Logger
{
public:
    /// Constructor.
    ///
    /// libdirs: List of directories to search for relative paths. The current
    /// directly will be tried first.
    CompilerContext(const string_list& libdirs);

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

    /// Enables additional debugging output.
    ///
    /// scanner: True to enable lexer debugging.
    ///
    /// parser: True to enable parser debugging.
    ///
    /// scopes: True to dump the scopes once built.
    void enableDebug(bool scanner, bool parser, bool scopes);

private:
    string_list _libdirs;

    /// We keep a global map of all module nodes we have instantiated so far,
    /// indexed by their path. This is for avoid duplicate imports, in
    /// particular when encountering cycles.
    std::map<string, shared_ptr<Module>> _modules;

    bool _dbg_scanner = false;
    bool _dbg_parser = false;
    bool _dbg_scopes = false;
};

}

#endif
