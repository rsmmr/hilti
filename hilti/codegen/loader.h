
#ifndef HILTI_CODEGEN_LOADER_H
#define HILTI_CODEGEN_LOADER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

/// Visitor that generates the code for loading the value of an HILTI
/// constant or expression. Note that this class should not be used directly,
/// the main frontend function is CodeGen::llvmValue().
class Loader : public CGVisitor<llvm::Value*>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   Loader(CodeGen* cg);
   virtual ~Loader();

   /// Returns the LLVM value resulting from evaluating a HILTI expression.
   ///
   /// expr: The expression to evaluate.
   ///
   /// coerce_to: If given, the expr is first coerced into this type before
   /// it's evaluated. It's assumed that the coercion is legal and supported.
   ///
   /// Returns: The computed LLVM value.
   llvm::Value* llvmValue(shared_ptr<Expression> expr, shared_ptr<hilti::Type> coerce_to = nullptr);

   /// Returns the LLVM value corresponding to a HILTI constant.
   ///
   /// constant: The constant.
   ///
   /// Returns: The computed LLVM value.
   llvm::Value* llvmValue(shared_ptr<Constant> constant);

protected:
   virtual void visit(expression::Variable* p) override;
   virtual void visit(expression::Parameter* p) override;
   virtual void visit(expression::CodeGen* c) override;

   virtual void visit(expression::Constant* e) override;
   virtual void visit(expression::Coerced* e) override;
   virtual void visit(expression::Function* f) override;

   virtual void visit(variable::Local* v) override;
   virtual void visit(variable::Global* v) override;

   virtual void visit(constant::Integer* c) override;
   virtual void visit(constant::String* s) override;
   virtual void visit(constant::Bool* b) override;
   virtual void visit(constant::Tuple* t) override;
   virtual void visit(constant::Reference* t) override;
};


}
}

#endif
