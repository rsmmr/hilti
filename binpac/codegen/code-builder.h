
#ifndef BINPAC_CODEGEN_CODE_BUILDER_H
#define BINPAC_CODEGEN_CODE_BUILDER_H

#include "common.h"
#include "cg-visitor.h"

namespace binpac {
namespace codegen {

/// Primary visitor for building HILTI code, including the overal module,
/// declarations, functions, statements, and expressions.
///
/// \note: The visitor arguments (expression, type) are only used for
/// evaluation expressions.
class CodeBuilder : public CGVisitor<shared_ptr<hilti::Node>, shared_ptr<Type>>
{
public:
    /// Constructor.
    ///
    /// cg: The code generator to use.
    CodeBuilder(CodeGen* cg);
    virtual ~CodeBuilder();

    /// Compiles a BinPAC statement into HILTI.
    ///
    /// stmt: The statement to compile
    void hiltiStatement(shared_ptr<Statement> stmt);

    /// Returns the HILTI epxression resulting from evaluating a BinPAC
    /// expression.
    ///
    /// expr: The expression to evaluate.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    ///
    /// Returns: The computed HILTI expression.
    shared_ptr<hilti::Expression> hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to = nullptr);

    /// Binds the $$ identifier to a given value.
    ///
    /// val: The value. Null unbinds.
    void hiltiBindDollarDollar(shared_ptr<hilti::Expression> val);

protected:
    /// Extracts the expression list from a call's tuple of parameters. It's
    /// an internal error if the expr is not constant, or does not have the
    /// right type.
    expression_list callParameters(shared_ptr<Expression> tupleop);

    /// Extracts a specific element from a call's tuple of parameters. It's
    /// an internal error if the expr is not constant, or does not have the
    /// right type; or if it doesn't have the required parameter.
    std::shared_ptr<Expression> callParameter(shared_ptr<Expression> tupleop, int i);

    void visit(Module* f) override;
    void visit(Function* f) override;
    void visit(Hook* h) override;

    void visit(constant::Address* a) override;
    void visit(constant::Bitset* b) override;
    void visit(constant::Bool* b) override;
    void visit(constant::Double* d) override;
    void visit(constant::Enum* e) override;
    // void visit(constant::Expression* e) override;
    void visit(constant::Integer* i) override;
    void visit(constant::Interval* i) override;
    void visit(constant::Network* n) override;
    void visit(constant::Port* p) override;
    void visit(constant::String* s) override;
    void visit(constant::Time* t) override;
    void visit(constant::Tuple* t) override;

    void visit(ctor::Bytes* b) override;
    void visit(ctor::List* l) override;
    void visit(ctor::Map* m) override;
    void visit(ctor::RegExp* r) override;
    void visit(ctor::Set* s) override;
    void visit(ctor::Vector* v) override;

    void visit(declaration::Constant* c) override;
    void visit(declaration::Function* f) override;
    void visit(declaration::Hook* h) override;
    void visit(declaration::Type* t) override;
    void visit(declaration::Variable* v) override;

    void visit(expression::Assign* a) override;
    void visit(expression::CodeGen* c) override;
    void visit(expression::Coerced* c) override;
    void visit(expression::Constant* c) override;
    void visit(expression::Ctor* c) override;
    void visit(expression::Default* d) override;
    void visit(expression::Function* f) override;
    void visit(expression::ID* i) override;
    void visit(expression::List* l) override;
    void visit(expression::MemberAttribute* m) override;
    void visit(expression::Module* m) override;
    void visit(expression::Parameter* p) override;
    void visit(expression::ParserState* p) override;
    void visit(expression::Type* t) override;
    void visit(expression::UnresolvedOperator* u) override;
    void visit(expression::Variable* v) override;

    void visit(statement::Block* b) override;
    void visit(statement::Expression* e) override;
    void visit(statement::ForEach* f) override;
    void visit(statement::IfElse* i) override;
    void visit(statement::NoOp* n) override;
    void visit(statement::Print* p) override;
    void visit(statement::Return* r) override;
    void visit(statement::Stop* s) override;
    void visit(statement::Try* t) override;
    void visit(statement::try_::Catch* c) override;

    void visit(variable::Global* v) override;
    void visit(variable::Local* v) override;

    /// Automatically generated visit() methods for ResolverOperator-derived
    /// classes.
    #include "autogen/operators/operators-expression-builder.h"

private:
    shared_ptr<hilti::Expression> _dollardollar = nullptr;

};

}
}

#endif
