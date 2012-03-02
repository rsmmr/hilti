
#ifndef HILTI_CODEGEN_UNPACKER_H
#define HILTI_CODEGEN_UNPACKER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

/// The result of Unpacker::llvmUnpack. The first element is the unpacked
/// instance, and the second an \c iterator<bytes> pointing one after the
/// last consumed byte.
typedef std::pair<llvm::Value*, llvm::Value*> UnpackResult;

typedef struct {
    /// type: The type of the object to be unpacked from the input. This must
    /// be a type::trait::Unpackable, or a reference of one.
    shared_ptr<Type> type;

    /// begin: A byte iterator marking the first input byte.
    llvm::Value* begin;

    /// end: A byte iterator marking the position one beyond the last
    /// consumable input byte. *end* may be null to indicate unpacking until
    /// the end of the bytes object is encountered.
    llvm::Value* end;

    /// fmt: Specifies the binary format of the input bytes as one of the
    /// ``Hilti::Packed`` labels.
    llvm::Value* fmt;

    /// arg: Additional format-specific parameter, required by some formats;
    /// nullptr if not.
    llvm::Value* arg;
} UnpackArgs;

/// Visitor that generates the code unpacking binary data into HILTI data
/// types.
class Unpacker : public CGVisitor<UnpackResult, UnpackArgs>
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
   /// the second a byte iterator pointing one beyond the last consumed
   /// input byte (which can't be further than *end*).
   UnpackResult llvmUnpack(const UnpackArgs& args);

protected:
   /// XXX
   void setResult(llvm::Value* value, llvm::Value* iter) {
       CGVisitor<UnpackResult, UnpackArgs>::setResult(std::make_pair(value, iter));
   }

   virtual void visit(type::Reference* t) override;
   virtual void visit(type::Bytes* t) override;
};

}
}

#endif
