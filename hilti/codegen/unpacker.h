
#ifndef HILTI_CODEGEN_UNPACKER_H
#define HILTI_CODEGEN_UNPACKER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

struct UnpackArgs {
    /// The type of the object to be unpacked from the input. This must be a
    /// type::trait::Unpackable, or a reference of one.
    shared_ptr<Type> type = nullptr;

    /// A byte iterator marking the first input byte. It will not have its
    /// ctor applied.
    llvm::Value* begin = nullptr;

    /// A byte iterator marking the position one beyond the last consumable
    /// input byte. *end* may be null to indicate unpacking until the end of
    /// the bytes object is encountered.  It will not have its ctor applied.
    llvm::Value* end = nullptr;

    /// Specifies the binary format of the input bytes as one of the
    /// ``Hilti::Packed`` labels.
    llvm::Value* fmt = nullptr;

    /// Additional format-specific parameter, required by some formats;
    /// nullptr if not.
    llvm::Value* arg = nullptr;

    /// Type of the additional format-specific parameter. nullptr if not
    /// used.
    shared_ptr<Type> arg_type = nullptr;

    /// A location associagted with the unpack operaton.
    Location location = Location::None;
};

struct UnpackResult {
    /// A pointer to an object of \a type's LLVM type storing the
    /// unpacked object. The value must have its cctor applied.
    llvm::Value* value_ptr = nullptr;

    /// A pointer to a bytes iterator where storing the position just after
    /// the last one consumed. The iterator must have its cctor applied.
    llvm::Value* iter_ptr = nullptr;
};

/// Visitor that generates the code unpacking binary data into HILTI data
/// types.
class Unpacker : public CGVisitor<int, UnpackArgs, UnpackResult>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   Unpacker(CodeGen* cg);
   virtual ~Unpacker();

   /// Unpacks an instance of a type from binary data.
   ///
   /// args: The arguments to the unpack operation.
   ///
   /// Returns: A pair in which the first value is the unpacked value, and
   /// the second a byte iterator pointing one beyond the last consumed input
   /// byte (which can't be further than *end*). Both tuples elements will
   /// have their cctor applied.
   UnpackResult llvmUnpack(const UnpackArgs& args);

protected:
   virtual void visit(type::Reference* t) override;
   virtual void visit(type::Bytes* t) override;
   virtual void visit(type::Integer* t) override;
};

}
}

#endif
