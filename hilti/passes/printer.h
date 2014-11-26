
#ifndef HILTI_PRINTER_H
#define HILTI_PRINTER_H

#include <ast/passes/printer.h>

#include "../common.h"
#include "../ast-info.h"

namespace hilti {
namespace passes {

/// Renders a HILTI AST back into HILTI source code.
class Printer : public ast::passes::Printer<AstInfo>
{
public:
   /// Constructor.
   ///
   /// out: Stream to write the source code representation to.
   ///
   /// single_line: If true, all line separator while be turned into space so
   /// that we get a single-line version of the output.
   ///
   /// cfg: If true, include control/data flow information in output if available.
   Printer(std::ostream& out, bool single_line = false, bool cfg = false);

   /// Don't print type attributes.
   void setOmitTypeAttributes(bool omit) { _omit_type_attributes = omit; }

   bool includeFlow() const;
   void printFlow(Statement* stmt, const string& prefix = "");
   bool printTypeID(Type* t);
   void printTypeAttributes(Type* t);
   void printAttributes(const AttributeSet& attrs);

protected:
   void visit(Module* m) override;
   void visit(ID* id) override;

   void visit(statement::Block* s) override;
   void visit(statement::Try* s) override;
   void visit(statement::instruction::Resolved* s) override;
   void visit(statement::instruction::Unresolved* s) override;
   void visit(statement::try_::Catch* s) override;
   void visit(statement::ForEach* s) override;

   void visit(declaration::Variable* v) override;
   void visit(declaration::Function* f) override;
   void visit(declaration::Type* t) override;

   void visit(type::function::Parameter* p) override;
   void visit(type::function::Result* r) override;

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
   void visit(type::HiltiFunction* t) override;
   void visit(type::Hook* t) override;
   void visit(type::IOSource* t) override;
   void visit(type::Integer* t) override;
   void visit(type::Interval* t) override;
   void visit(type::Iterator* t) override;
   void visit(type::Label* t) override;
   void visit(type::List* t) override;
   void visit(type::Map* t) override;
   void visit(type::MatchTokenState* t) override;
   void visit(type::Network* t) override;
   void visit(type::Overlay* t) override;
   void visit(type::OptionalArgument* t) override;
   void visit(type::Port* t) override;
   void visit(type::Reference* t) override;
   void visit(type::RegExp* t) override;
   void visit(type::Scope* t) override;
   void visit(type::Set* t) override;
   void visit(type::String* t) override;
   void visit(type::Struct* t) override;
   void visit(type::Time* t) override;
   void visit(type::Timer* t) override;
   void visit(type::TimerMgr* t) override;
   void visit(type::Tuple* t) override;
   void visit(type::TypeType* t) override;
   void visit(type::Union* t) override;
   void visit(type::Unknown* t) override;
   void visit(type::Unset* t) override;
   void visit(type::Vector* t) override;
   void visit(type::Void* t) override;

   void visit(expression::Block* e) override;
   void visit(expression::CodeGen* e) override;
   void visit(expression::Coerced* e) override;
   void visit(expression::Constant* e) override;
   void visit(expression::Ctor* e) override;
   void visit(expression::Function* e) override;
   void visit(expression::ID* e) override;
   void visit(expression::Module* e) override;
   void visit(expression::Parameter* e) override;
   void visit(expression::Type* e) override;
   void visit(expression::Default* e) override;
   void visit(expression::Variable* e) override;

   void visit(constant::Address* c) override;
   void visit(constant::Bitset* c) override;
   void visit(constant::Bool* c) override;
   void visit(constant::CAddr* c) override;
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
   void visit(constant::Union* c) override;

   void visit(ctor::Bytes* c) override;
   void visit(ctor::List* c) override;
   void visit(ctor::Map* c) override;
   void visit(ctor::RegExp* c) override;
   void visit(ctor::Set* c) override;
   void visit(ctor::Vector* c) override;
   void visit(ctor::Callable* c) override;

private:
   Module* _module = nullptr;
   bool _cfg;
   bool _omit_type_attributes = false;
};

}
}

#endif
