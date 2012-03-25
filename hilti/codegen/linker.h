
#ifndef HILTI_CODEGEN_LINKER_H
#define HILTI_CODEGEN_LINKER_H

#include "../id.h"
#include "common.h"

namespace hilti {
namespace codegen {

/// HILTI's custom linker. It links together a set of compiled HILTI modules
/// and inserts additional information and code used by the HILTI run-time.
class Linker : public ast::Logger {
public:
   /// Constructor.
   ///
   /// paths: List of directories to search for native libraries.
   Linker(const path_list& paths) : ast::Logger("codegen::Linker") {
       _paths = paths;
   };

   /// Adds a native library to be linked in when link() is called.
   ///
   /// name: The name of library, like you would give it with ``-l<name>``.
   /// It will searched in the paths given to the constructor, plus any
   /// system linker paths.
   void addNativeLibrary(const string& name) { _libs.push_back(name); }

   /// Adds an LLVM bitcode archive to be linked in when link() is called.
   ///
   /// path: The full path to the ``*.bca`` file.
   void addBitcodeArchive(const string& path)  { _bcas.push_back(path); }

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
   /// Returns: The composite module. Passes ownership to caller.
   llvm::Module* link(string output, const std::list<llvm::Module*>& modules, bool verify, int debug_cg);

private:
   bool isHiltiModule(llvm::Module* module);
   void addModuleInfo(const std::list<string>& module_names, llvm::Module* module);
   void addGlobalsInfo(const std::list<string>& module_names, llvm::Module* module);
   void joinFunctions(const char* new_func, const char* meta, const std::list<string>& module_names, llvm::Module* module);
   void makeHooks(const std::list<string>& module_names, llvm::Module* module);
   void error(const llvm::Linker& linker, const string& file);

   path_list _paths;
   path_list _libs;
   path_list _bcas;
};

}
}

#endif
