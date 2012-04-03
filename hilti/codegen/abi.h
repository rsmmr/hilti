
#ifndef HILTI_CODEGEN_ABI_H
#define HILTI_CODEGEN_ABI_H

#include "common.h"

namespace llvm { class Value; }

namespace hilti {
namespace codegen {

/// Nase class providing the ABI interface.
class ABI
{
public:
   typedef llvm::Value* argument;
   typedef std::vector<argument> argument_list;

   typedef llvm::Type* argument_type;
   typedef std::vector<argument_type> argument_type_list;

   /// XXX
   virtual void adaptFunctionType(argument_type* rtype, argument_type_list* args)
        { /* Nop by default */ }

   /// XXX
   virtual void adaptFunctionCall(argument_list* args)
        { /* Nop by default */ }

   /// XXX
   virtual llvm::Value* getFunctionResult(llvm::CallInst* call) {
        // Pass through by default
        return call;
   }

   /// XXX
   const string& targetTriple() const { return _triple; }

   /// XXX
   static unique_ptr<ABI> createABI(llvm::Module* module);

protected:
   ABI() {}

private:
   string _triple;
};

namespace abi {

class X86_64 : public ABI
{
public:
   enum Flavor { DEFAULT, DARWIN };

   X86_64(Flavor flavor) { _flavor = flavor; }

   void adaptFunctionType(ABI::argument_type* rtype, ABI::argument_type_list* args) override;
   void adaptFunctionCall(argument_list* args) override;
   llvm::Value* getFunctionResult(llvm::CallInst* call) override;

private:
   Flavor _flavor;
};

}
}
}

#endif
