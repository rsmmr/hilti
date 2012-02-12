#ifndef HILTI_CODEGEN_STORER_H
#define HILTI_CODEGEN_STORER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

/// Visitor that generates the code for storing an LLVM value into a target
/// described by a HILTI expression. Note that this class should not be used
/// directly, the main frontend function is CodeGen::llvmStore().
class Storer : public CGVisitor<int, llvm::Value*, bool>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   Storer(CodeGen* cg);
   virtual ~Storer();

   /// Stores an LLVM value into a target referenced by a HILTI expression.
   ///
   /// target: The target of the store.
   ///
   /// value: The value to store.
   void llvmStore(shared_ptr<Expression> target, llvm::Value* value, bool plusone);

protected:
   void visit(expression::Variable* v) override;
   void visit(expression::Parameter* p) override;
   void visit(expression::CodeGen* c) override;

   void visit(variable::Local* v) override;
   void visit(variable::Global* v) override;
};


}
}

#endif
