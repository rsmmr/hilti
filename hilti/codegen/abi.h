
#ifndef HILTI_CODEGEN_ABI_H
#define HILTI_CODEGEN_ABI_H

#include "common.h"
#include "type-builder.h"

// This is generated in build/.
#include <codegen/libffi/ffi.h>

namespace llvm { class Value; }

namespace hilti {
namespace codegen {

class CodeGen;

/// Nase class providing the ABI interface.
class ABI
{
public:
   typedef std::vector<std::pair<string, llvm::Type*>> arg_list;

   /// XXX
   virtual llvm::Function* createFunction(const string& name, llvm::Type* rtype, const arg_list& args, llvm::GlobalValue::LinkageTypes linkage, llvm::Module* module) = 0;

   /// XXX
   virtual llvm::Value* createCall(llvm::Function *callee, std::vector<llvm::Value *> args) = 0;

   /// XXX
   CodeGen* cg() const { return _cg; }

   /// XXX
   const string& targetTriple() const { return _triple; }

   /// XXX
   static unique_ptr<ABI> createABI(CodeGen* cg);

protected:
   ABI() {}

   typedef std::vector<llvm::Type*> argument_type_list;
   ffi_cif* getCIF(const string& name, llvm::Type* rtype, const ABI::argument_type_list& args);

private:
   string _triple;
   CodeGen* _cg = nullptr;

   typedef std::map<string, ffi_cif*> cif_map;
   cif_map _cifs;
};

namespace abi {

/// ABI for X86_64.
///
/// \todo The only ABI-specific transformation we currently do is adjusting
/// the return value to be passed via a hidding parameter if it needs to be.
/// We don't handle call arguments yet that need to be passed in memory.
class X86_64 : public ABI
{
public:
   enum Flavor { DEFAULT, DARWIN };

   X86_64(Flavor flavor) { _flavor = flavor; }

   llvm::Function* createFunction(const string& name, llvm::Type* rtype, const ABI::arg_list& args, llvm::GlobalValue::LinkageTypes linkage, llvm::Module* module) override;
   llvm::Value* createCall(llvm::Function *callee, std::vector<llvm::Value *> args) override;

private:
   bool returnInMemory(ffi_cif* cif);

   Flavor _flavor;
};

}
}
}

#endif
