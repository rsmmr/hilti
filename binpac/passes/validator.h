
#ifndef BINPAC_PASSES_VALIDATOR_H
#define BINPAC_PASSES_VALIDATOR_H

#include <ast/pass.h>

#include "../ast-info.h"
#include "../common.h"

namespace binpac {
namespace passes {

/// Verifies the semantic of an AST.
class Validator : public ast::Pass<AstInfo> {
public:
    Validator();
    virtual ~Validator();

    /// Verifies a given module.
    ///
    /// Returns: True if no errors were found.
    bool run(shared_ptr<ast::NodeBase> ast) override;

    /// Helper to report a type mismatch error. This calls error().
    ///
    /// node: The node to get the error location from.
    ///
    /// msg: A message to print.
    ///
    /// have: The wrong type that was given to us.
    ///
    /// want: The type we expected instead.
    void wrongType(ast::NodeBase* node, const string& msg, shared_ptr<Type> have,
                   shared_ptr<Type> want);

    /// Helper to report a type mismatch error. This calls error().
    ///
    /// node: The node to get the error location from.
    ///
    /// msg: A message to print.
    ///
    /// have: The wrong type that was given to us.
    ///
    /// want: The type we expected instead.
    void wrongType(shared_ptr<Node> node, const string& msg, shared_ptr<Type> have,
                   shared_ptr<Type> want);

    /// Reports an error if type is not valid as a function return type.
    ///
    /// node: The node to get the error location from.
    ///
    /// type: The type to check.
    ///
    /// Returns: False if an error was found.
    bool checkReturnType(ast::NodeBase* node, shared_ptr<Type> type);

    /// Reports an error if type is not valid as a function parameter.
    ///
    /// node: The node to get the error location from.
    ///
    /// type: The type to check.
    ///
    /// Returns: False if an error was found.
    bool checkParameterType(ast::NodeBase* node, shared_ptr<Type> type);

    /// Reports an error if type is not valid as a variable type.
    ///
    /// node: The node to get the error location from.
    ///
    /// type: The type to check.
    ///
    /// Returns: False if an error was found.
    bool checkVariableType(ast::NodeBase* node, shared_ptr<Type> type);

protected:
    void visit(Attribute* a) override;
    void visit(AttributeSet* a) override;
    void visit(Constant* c) override;
    void visit(Ctor* c) override;
    void visit(Declaration* d) override;
    void visit(Expression* e) override;
    void visit(Function* f) override;
    void visit(Hook* h) override;
    void visit(ID* i) override;
    void visit(Module* m) override;
    void visit(Statement* s) override;
    void visit(Type* t) override;
    void visit(Variable* v) override;
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
    void visit(ctor::Unit* u) override;
    void visit(ctor::Vector* v) override;
    void visit(declaration::Constant* c) override;
    void visit(declaration::Function* f) override;
    void visit(declaration::Hook* h) override;
    void visit(declaration::Type* t) override;
    void visit(declaration::Variable* v) override;
    void visit(expression::Assign* a) override;
    void visit(expression::CodeGen* c) override;
    void visit(expression::Coerced* c) override;
    void visit(expression::Conditional* c) override;
    void visit(expression::Constant* c) override;
    void visit(expression::Ctor* c) override;
    void visit(expression::Function* f) override;
    void visit(expression::ID* i) override;
    void visit(expression::List* l) override;
    void visit(expression::ListComprehension* c) override;
    void visit(expression::Module* m) override;
    void visit(expression::Parameter* p) override;
    void visit(expression::ParserState* p) override;
    void visit(expression::ResolvedOperator* r) override;
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
    void visit(statement::Try* t) override;
    void visit(statement::try_::Catch* c) override;
    void visit(type::Address* a) override;
    void visit(type::Any* a) override;
    void visit(type::Bitfield* b) override;
    void visit(type::Bitset* b) override;
    void visit(type::Bool* b) override;
    void visit(type::Bytes* b) override;
    void visit(type::CAddr* c) override;
    void visit(type::Double* d) override;
    void visit(type::Enum* e) override;
    void visit(type::Exception* e) override;
    void visit(type::File* f) override;
    void visit(type::Function* f) override;
    void visit(type::Hook* h) override;
    void visit(type::Integer* i) override;
    void visit(type::Interval* i) override;
    void visit(type::Iterator* i) override;
    void visit(type::List* l) override;
    void visit(type::Map* m) override;
    void visit(type::Module* m) override;
    void visit(type::Network* n) override;
    void visit(type::OptionalArgument* o) override;
    void visit(type::Port* p) override;
    void visit(type::RegExp* r) override;
    void visit(type::Set* s) override;
    void visit(type::Sink* s) override;
    void visit(type::String* s) override;
    void visit(type::Time* t) override;
    void visit(type::Tuple* t) override;
    void visit(type::TypeByName* t) override;
    void visit(type::TypeType* t) override;
    void visit(type::Unit* u) override;
    void visit(type::Unknown* u) override;
    void visit(type::Unset* u) override;
    void visit(type::Vector* v) override;
    void visit(type::Void* v) override;
    void visit(type::function::Parameter* p) override;
    void visit(type::function::Result* r) override;
    void visit(type::iterator::Bytes* b) override;
    void visit(type::iterator::List* l) override;
    void visit(type::iterator::Map* r) override;
    void visit(type::iterator::Set* s) override;
    void visit(type::iterator::Vector* v) override;
    void visit(type::unit::Item* i) override;
    void visit(type::unit::item::Field* f) override;
    void visit(type::unit::item::GlobalHook* g) override;
    void visit(type::unit::item::Property* p) override;
    void visit(type::unit::item::Variable* v) override;
    void visit(type::unit::item::field::Container* f) override;
    void visit(type::unit::item::field::container::List* f) override;
    void visit(type::unit::item::field::container::Vector* f) override;
    void visit(type::unit::item::field::Constant* c) override;
    void visit(type::unit::item::field::Ctor* r) override;
    void visit(type::unit::item::field::Switch* s) override;
    void visit(type::unit::item::field::AtomicType* t) override;
    void visit(type::unit::item::field::Unit* t) override;
    void visit(type::unit::item::field::Unknown* f) override;
    void visit(type::unit::item::field::switch_::Case* c) override;
    void visit(variable::Global* g) override;
    void visit(variable::Local* l) override;
};
}
}

#endif
