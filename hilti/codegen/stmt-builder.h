
#ifndef HILTI_CODEGEN_STMT_BUILDER_H
#define HILTI_CODEGEN_STMT_BUILDER_H

#include "../common.h"
#include "../visitor.h"
#include "../instructions/all.h"

#include "common.h"
#include "codegen.h"

namespace hilti {
namespace codegen {

/// Visitor that generates the code for the execution of statemens. Note that
/// this class should not be used directly, it's used internally by CodeGen.
class StatementBuilder : public CGVisitor<>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   StatementBuilder(CodeGen* cg);
   virtual ~StatementBuilder();

   /// Generates the code for a statement.
   ///
   /// stmt: The statement.
   void llvmStatement(shared_ptr<Statement> stmt);

protected:
   void visit(statement::Block* b) override;

   void visit(declaration::Function* f);
   void visit(declaration::Variable* v);

   void visit(statement::instruction::integer::Add* i) override;
   void visit(statement::instruction::integer::Sub* i) override;

   void visit(statement::instruction::flow::ReturnResult* i) override;
   void visit(statement::instruction::flow::ReturnVoid* i) override;
   void visit(statement::instruction::flow::CallVoid* i) override;
   void visit(statement::instruction::flow::CallResult* i) override;

   void prepareCall(shared_ptr<Expression> func, shared_ptr<Expression> args, CodeGen::expr_list* call_params);
};

}
}

#endif
