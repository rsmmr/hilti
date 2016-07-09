
#ifndef HILTI_CODEGEN_LINKER_H
#define HILTI_CODEGEN_LINKER_H

#include "../id.h"
#include "common.h"

namespace hilti {

class CompilerContext;
class Options;

namespace codegen {

/// HILTI's custom linker. It links together a set of compiled HILTI modules
/// and inserts additional information and code used by the HILTI run-time.
///
/// TODO: We should move all the paths and libraries over into the Options
/// class.
class Linker : public ast::Logger {
public:
    /// Constructor.
    ///
    /// paths: List of directories to search for native libraries.
    Linker(CompilerContext* ctx, const path_list& paths) : ast::Logger("codegen::Linker")
    {
        _ctx = ctx;
        _paths = paths;
    };

    /// Returns the compiler context the code generator is used with.
    ///
    CompilerContext* context() const;

    /// Returns the LLVM context the linker is using.
    llvm::LLVMContext& llvmContext();

    /// Returns the options in effecty for code generation. This is a
    /// convienience method that just forwards to the current context.
    const Options& options() const;

    /// Adds a native library to be linked in when link() is called.
    ///
    /// name: The name of library, like you would give it with ``-l<name>``.
    /// It will searched in the paths given to the constructor, plus any
    /// system linker paths.
    void addNativeLibrary(const string& name)
    {
        _natives.push_back(name);
    }

    /// Adds an LLVM bitcode file to be linked in when link() is called.
    ///
    /// path: The full path to the ``*.bc`` file.
    void addBitcodeFile(const string& path)
    {
        _bcs.push_back(path);
    }

    /// Links a set of compiled HILTI modules together.
    ///
    /// output: The name of the output module.
    ///
    /// modules: The set of HILTI (and potentially non-HILTI) modules to link.
    /// Takes ownership of these.
    ///
    /// verify: True if resulting LLVM module should be verified for
    /// correctness. Should normally be on except for debugging.
    ///
    /// debug_cg: The debug level for the link process. The higher the level,
    /// the more debug output the linking may generate.
    ///
    /// Returns: The composite module.
    std::unique_ptr<llvm::Module> link(string output,
                                       std::list<std::unique_ptr<llvm::Module>>& modules);

private:
    bool isHiltiModule(llvm::Module* module);
    void addModuleInfo(llvm::Module* module, const std::list<string>& module_names);
    void addGlobalsInfo(llvm::Module* module, const std::list<string>& module_names);
    void joinFunctions(llvm::Module* dst, const char* new_func, const char* meta,
                       llvm::FunctionType* default_ftype, const std::list<string>& module_names,
                       llvm::Module* module);
    void makeHooks(llvm::Module* module, const std::list<string>& module_names);
    void fatalError(const string& where, const string& file = "", const string& error = "");

    // These following three abort directly on error.
    void linkInModule(llvm::Linker* linker, std::unique_ptr<llvm::Module> module);
    void linkInNativeLibrary(llvm::Linker* linker, const string& library);
    // void linkInArchive(llvm::Linker* linker, const string& library);

    CompilerContext* _ctx;
    path_list _paths;
    path_list _natives;
    path_list _bcs;
};
}
}

#endif
