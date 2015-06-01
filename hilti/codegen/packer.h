
#ifndef HILTI_CODEGEN_PACKER_H
#define HILTI_CODEGEN_PACKER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

struct PackArgs {
    /// The type of the object to be packed. This must be a
    /// type::trait::Packable, or a reference of one.
    shared_ptr<Type> type = nullptr;

    /// The value being packed.
    llvm::Value* value = nullptr;

    /// Specifies the binary format of the packed bytes as one of the
    /// ``Hilti::Packed`` labels.
    llvm::Value* fmt = nullptr;

    /// Additional format-specific parameter, required by some formats;
    /// nullptr if not.
    llvm::Value* arg = nullptr;

    /// Type of the additional format-specific parameter. nullptr if not
    /// used.
    shared_ptr<Type> arg_type = nullptr;

    /// A location associagted with the Pack operaton.
    Location location = Location::None;
};

// A bytes instance.
typedef llvm::Value* PackResult;

/// Visitor that generates the code packing HILTI data into binary.
class Packer : public CGVisitor<int, PackArgs, PackResult>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   Packer(CodeGen* cg);
   virtual ~Packer();

   /// Packs an instance of a type into binary data.
   ///
   /// args: The arguments to the Pack operation.
   ///
   /// Returns: The packed value.
   PackResult llvmPack(const PackArgs& args);

protected:
   virtual void visit(type::Reference* t) override;
   virtual void visit(type::Bytes* t) override;
   virtual void visit(type::Integer* t) override;
   virtual void visit(type::Address* t) override;
   virtual void visit(type::Port* t) override;
   virtual void visit(type::Bool* t) override;
   virtual void visit(type::Double* t) override;
};

}
}

#endif
