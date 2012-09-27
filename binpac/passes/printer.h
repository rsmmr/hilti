
#ifndef BINPAC_PASSES_PRINTER_H
#define BINPAC_PASSES_PRINTER_H

#include <ast/passes/printer.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {
namespace passes {

/// Renders a BinPAC AST back into BinPAC source code.
class Printer : public ast::passes::Printer<AstInfo>
{
public:
    /// Constructor.
    ///
    /// out: Stream to write the source code representation to.
    ///
    /// single_line: If true, all line separator while be turned into space so
    /// that we get a single-line version of the output.
    Printer(std::ostream& out, bool single_line = false);

protected:
    void visit(Attribute* a) override;
    void visit(AttributeSet* a) override;
    void visit(Function* f) override;
    void visit(Hook* f) override;
    void visit(ID* i) override;
    void visit(Module* m) override;
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
    void visit(declaration::Hook* f) override;
    void visit(declaration::Type* t) override;
    void visit(declaration::Variable* v) override;
    void visit(expression::CodeGen* c) override;
    void visit(expression::Coerced* c) override;
    void visit(expression::Constant* c) override;
    void visit(expression::Ctor* c) override;
    void visit(expression::Function* f) override;
    void visit(expression::ID* i) override;
    void visit(expression::List* l) override;
    void visit(expression::MemberAttribute* m) override;
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
    void visit(statement::Stop* r) override;
    void visit(statement::Try* t) override;
    void visit(statement::try_::Catch* c) override;
    void visit(type::Address* a) override;
    void visit(type::Any* a) override;
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
    void visit(type::unit::item::GlobalHook* ) override;
    void visit(type::unit::item::Property* ) override;
    void visit(type::unit::item::Variable* ) override;
    void visit(type::unit::item::field::Constant* ) override;
    void visit(type::unit::item::field::Ctor* ) override;
    void visit(type::unit::item::field::Switch* ) override;
    void visit(type::unit::item::field::AtomicType* ) override;
    void visit(type::unit::item::field::Unit* ) override;
    void visit(type::unit::item::field::switch_::Case* ) override;
    void visit(variable::Global* g) override;
    void visit(variable::Local* l) override;
};

}

}

#endif
