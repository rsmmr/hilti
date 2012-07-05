
#ifndef HILTI_VALIDATOR_H
#define HILTI_VALIDATOR_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Verifies the semantic of an AST.
class Validator : public Pass<>
{
public:
   Validator() : Pass<>("Validator") {}
   virtual ~Validator();

   /// Verifies a given module.
   ///
   /// Returns: True if no errors were found.
   bool run(shared_ptr<Node> module) override {
       return processAllPreOrder(module);
   }

   /// Helper to report a type mismatch error. This calls error().
   ///
   /// node: The node to get the error location from.
   ///
   /// msg: A message to print.
   ///
   /// have: The wrong type that was given to us.
   ///
   /// want: The type we expected instead.
   void wrongType(ast::NodeBase* node, const string& msg, shared_ptr<Type> have, shared_ptr<Type> want);

   /// Helper to report a type mismatch error. This calls error().
   ///
   /// node: The node to get the error location from.
   ///
   /// msg: A message to print.
   ///
   /// have: The wrong type that was given to us.
   ///
   /// want: The type we expected instead.
   void wrongType(shared_ptr<Node> node, const string& msg, shared_ptr<Type> have, shared_ptr<Type> want);

   /// Reports an error if type is not valid as a function return type.
   ///
   /// node: The node to get the error location from.
   ///
   /// type: The type to check.
   ///
   /// Returns: False if an error was found.
   bool validReturnType(ast::NodeBase* node, shared_ptr<Type> type, type::function::CallingConvention cc);

   /// Reports an error if type is not valid as a function parameter.
   ///
   /// node: The node to get the error location from.
   ///
   /// type: The type to check.
   ///
   /// Returns: False if an error was found.
   bool validParameterType(ast::NodeBase* node, shared_ptr<Type> type, type::function::CallingConvention cc);

   /// Reports an error if type is not valid as a variable type.
   ///
   /// node: The node to get the error location from.
   ///
   /// type: The type to check.
   ///
   /// Returns: False if an error was found.
   bool validVariableType(ast::NodeBase* node, shared_ptr<Type> type);

protected:
   void visit(Module* m) override;
   void visit(ID* id) override;

   void visit(statement::Block* s) override;
   void visit(statement::Try* s) override;
   void visit(statement::instruction::Resolved* s) override;
   void visit(statement::instruction::Unresolved* s) override;
   void visit(statement::try_::Catch* s) override;
   void visit(statement::ForEach* s) override;
   void visit(statement::instruction::flow::ReturnResult* s) override;
   void visit(statement::instruction::flow::ReturnVoid* s) override;

   void visit(statement::instruction::thread::GetContext* s) override;
   void visit(statement::instruction::thread::SetContext* s) override;
   void visit(statement::instruction::thread::Schedule* s) override;
   void visit(statement::instruction::flow::CallResult* s) override;
   void visit(statement::instruction::flow::CallVoid* s) override;

   void visit(declaration::Variable* v) override;
   void visit(declaration::Type* t) override;
   void visit(declaration::Function* f) override;
   void visit(declaration::Hook* f) override;

   void visit(type::function::Parameter* p) override;
   void visit(type::overlay::Field* f) override;

   void visit(type::Address* t) override;
   void visit(type::Any* t) override;
   void visit(type::Bitset* t) override;
   void visit(type::Bool* t) override;
   void visit(type::Bytes* t) override;
   void visit(type::CAddr* t) override;
   void visit(type::Callable* t) override;
   void visit(type::Channel* t) override;
   void visit(type::Classifier* t) override;
   void visit(type::Double* t) override;
   void visit(type::Enum* t) override;
   void visit(type::Exception* t) override;
   void visit(type::File* t) override;
   void visit(type::Function* t) override;
   void visit(type::Hook* t) override;
   void visit(type::IOSource* t) override;
   void visit(type::Integer* t) override;
   void visit(type::Interval* t) override;
   void visit(type::Iterator* t) override;
   void visit(type::List* t) override;
   void visit(type::Map* t) override;
   void visit(type::MatchTokenState* t) override;
   void visit(type::Network* t) override;
   void visit(type::Overlay* t) override;
   void visit(type::Port* t) override;
   void visit(type::Reference* t) override;
   void visit(type::RegExp* t) override;
   void visit(type::Set* t) override;
   void visit(type::String* t) override;
   void visit(type::Struct* t) override;
   void visit(type::Time* t) override;
   void visit(type::Timer* t) override;
   void visit(type::TimerMgr* t) override;
   void visit(type::Tuple* t) override;
   void visit(type::TypeType* t) override;
   void visit(type::Unknown* t) override;
   void visit(type::Vector* t) override;
   void visit(type::Void* t) override;

   void visit(expression::Block* e) override;
   void visit(expression::CodeGen* e) override;
   void visit(expression::Constant* e) override;
   void visit(expression::Ctor* e) override;
   void visit(expression::Function* e) override;
   void visit(expression::ID* e) override;
   void visit(expression::Module* e) override;
   void visit(expression::Parameter* e) override;
   void visit(expression::Type* e) override;
   void visit(expression::Variable* e) override;

   void visit(constant::Address* c) override;
   void visit(constant::Bitset* c) override;
   void visit(constant::Bool* c) override;
   void visit(constant::Double* c) override;
   void visit(constant::Enum* c) override;
   void visit(constant::Integer* c) override;
   void visit(constant::Interval* c) override;
   void visit(constant::Label* c) override;
   void visit(constant::Network* c) override;
   void visit(constant::Port* c) override;
   void visit(constant::Reference* c) override;
   void visit(constant::String* c) override;
   void visit(constant::Time* c) override;
   void visit(constant::Tuple* c) override;
   void visit(constant::Unset* c) override;

   void visit(ctor::Bytes* c) override;
   void visit(ctor::List* c) override;
   void visit(ctor::Map* c) override;
   void visit(ctor::RegExp* c) override;
   void visit(ctor::Set* c) override;
   void visit(ctor::Vector* c) override;
};

}
}

#endif
