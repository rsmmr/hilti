
#ifndef HILTI_CODEGEN_COERCER_H
#define HILTI_CODEGEN_COERCER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

/// Visitor that generates the code for coercing an LLVM value from one HILTI
/// type into another. Note that this class should not be used directly, the
/// frontend is CodeGen::llvmCoerceTo.
class Coercer : public CGVisitor<llvm::Value*, llvm::Value*, shared_ptr<hilti::Type>>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   Coercer(CodeGen* cg);
   virtual ~Coercer();

   /// Coerces an LLVM value from one type into another. It assumes that the
   /// coercion is legal and supported. If not, results are undefined.
   ///
   /// value: The value to coerce.
   ///
   /// src: The source type.
   ///
   /// dst: The destination type.
   ///
   /// Returns: The coerce value.
   llvm::Value* llvmCoerceTo(llvm::Value* value, shared_ptr<hilti::Type> src, shared_ptr<hilti::Type> dst);

protected:
   void visit(type::Address* t) override;
   void visit(type::Integer* t) override;
   void visit(type::Tuple* t) override;
   void visit(type::Reference* t) override;
   void visit(type::CAddr* t) override;
};


}
}

#endif
