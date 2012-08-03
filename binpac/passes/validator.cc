
#include "validator.h"
#include "type.h"

using namespace binpac;
using namespace binpac::passes;

Validator::Validator() : Pass<AstInfo>("Validator")
{
}

bool Validator::run(shared_ptr<ast::NodeBase> node)
{
    return processAllPreOrder(node);
}

Validator::~Validator()
{
}

void Validator::wrongType(ast::NodeBase* node, const string& msg, shared_ptr<Type> have, shared_ptr<Type> want)
{
    string m = msg;

    if ( have )
        m += string("\n    Type given:    " + have->render());

    if ( want )
        m += string("\n    Type expected: " + want->render());

    return error(node, m);
}

void Validator::wrongType(shared_ptr<Node> node, const string& msg, shared_ptr<Type> have, shared_ptr<Type> want)
{
    return wrongType(node.get(), msg, have, want);
}

bool Validator::checkReturnType(ast::NodeBase* node, shared_ptr<Type> type)
{
    // TODO.
    return true;
}

bool Validator::checkParameterType(ast::NodeBase* node, shared_ptr<Type> type)
{
    // TODO.
    return true;
}

bool Validator::checkVariableType(ast::NodeBase* node, shared_ptr<Type> type)
{
    // TODO.
    return true;
}

void Validator::visit(Attribute* a)
{
}

void Validator::visit(AttributeSet* a)
{
}

void Validator::visit(Constant* c)
{
}

void Validator::visit(Ctor* c)
{
}

void Validator::visit(Declaration* d)
{
}

void Validator::visit(Expression* e)
{
}

void Validator::visit(Function* f)
{
}

void Validator::visit(Hook* h)
{
}

void Validator::visit(ID* i)
{
}

void Validator::visit(Module* m)
{
}

void Validator::visit(Statement* s)
{
}

void Validator::visit(Type* t)
{
}

void Validator::visit(Variable* v)
{
}

void Validator::visit(constant::Address* a)
{
}

void Validator::visit(constant::Bitset* b)
{
}

void Validator::visit(constant::Bool* b)
{
}

void Validator::visit(constant::Double* d)
{
}

void Validator::visit(constant::Enum* e)
{
}

void Validator::visit(constant::Expression* e)
{
}

void Validator::visit(constant::Integer* i)
{
}

void Validator::visit(constant::Interval* i)
{
}

void Validator::visit(constant::Network* n)
{
}

void Validator::visit(constant::Port* p)
{
}

void Validator::visit(constant::String* s)
{
}

void Validator::visit(constant::Time* t)
{
}

void Validator::visit(constant::Tuple* t)
{
}

void Validator::visit(ctor::Bytes* b)
{
}

void Validator::visit(ctor::List* l)
{
}

void Validator::visit(ctor::Map* m)
{
}

void Validator::visit(ctor::RegExp* r)
{
}

void Validator::visit(ctor::Set* s)
{
}

void Validator::visit(ctor::Vector* v)
{
}

void Validator::visit(declaration::Constant* c)
{
}

void Validator::visit(declaration::Function* f)
{
}

void Validator::visit(declaration::Hook* h)
{
}

void Validator::visit(declaration::Type* t)
{
}

void Validator::visit(declaration::Variable* v)
{
}

void Validator::visit(expression::CodeGen* c)
{
}

void Validator::visit(expression::Coerced* c)
{
}

void Validator::visit(expression::Constant* c)
{
}

void Validator::visit(expression::Ctor* c)
{
}

void Validator::visit(expression::Function* f)
{
}

void Validator::visit(expression::ID* i)
{
}

void Validator::visit(expression::List* l)
{
}

void Validator::visit(expression::Module* m)
{
}

void Validator::visit(expression::Parameter* p)
{
}

void Validator::visit(expression::ResolvedOperator* r)
{
}

void Validator::visit(expression::Type* t)
{
}

void Validator::visit(expression::UnresolvedOperator* u)
{
}

void Validator::visit(expression::Variable* v)
{
}

void Validator::visit(statement::Block* b)
{
}

void Validator::visit(statement::Expression* e)
{
}

void Validator::visit(statement::ForEach* f)
{
}

void Validator::visit(statement::IfElse* i)
{
}

void Validator::visit(statement::NoOp* n)
{
}

void Validator::visit(statement::Print* p)
{
}

void Validator::visit(statement::Return* r)
{
}

void Validator::visit(statement::Try* t)
{
}

void Validator::visit(statement::try_::Catch* c)
{
}

void Validator::visit(type::Address* a)
{
}

void Validator::visit(type::Any* a)
{
}

void Validator::visit(type::Bitset* b)
{
}

void Validator::visit(type::Block* b)
{
}

void Validator::visit(type::Bool* b)
{
}

void Validator::visit(type::Bytes* b)
{
}

void Validator::visit(type::CAddr* c)
{
}

void Validator::visit(type::Double* d)
{
}

void Validator::visit(type::Enum* e)
{
}

void Validator::visit(type::Exception* e)
{
}

void Validator::visit(type::File* f)
{
}

void Validator::visit(type::Function* f)
{
}

void Validator::visit(type::Hook* h)
{
}

void Validator::visit(type::Integer* i)
{
}

void Validator::visit(type::Interval* i)
{
}

void Validator::visit(type::Iterator* i)
{
}

void Validator::visit(type::List* l)
{
}

void Validator::visit(type::Map* m)
{
}

void Validator::visit(type::Module* m)
{
}

void Validator::visit(type::Network* n)
{
}

void Validator::visit(type::OptionalArgument* o)
{
}

void Validator::visit(type::Port* p)
{
}

void Validator::visit(type::RegExp* r)
{
}

void Validator::visit(type::Set* s)
{
}

void Validator::visit(type::String* s)
{
}

void Validator::visit(type::Time* t)
{
}

void Validator::visit(type::Tuple* t)
{
}

void Validator::visit(type::TypeByName* t)
{
}

void Validator::visit(type::TypeType* t)
{
}

void Validator::visit(type::Unit* u)
{
}

void Validator::visit(type::Unknown* u)
{
}

void Validator::visit(type::Unset* u)
{
}

void Validator::visit(type::Vector* v)
{
}

void Validator::visit(type::Void* v)
{
}

void Validator::visit(type::function::Parameter* p)
{
}

void Validator::visit(type::function::Result* r)
{
}

void Validator::visit(type::iterator::Bytes* b)
{
}

void Validator::visit(type::iterator::List* l)
{
}

void Validator::visit(type::iterator::Regexp* r)
{
}

void Validator::visit(type::iterator::Set* s)
{
}

void Validator::visit(type::iterator::Vector* v)
{
}

void Validator::visit(type::unit::Item* i)
{
}

void Validator::visit(type::unit::item::Field* f)
{
}

void Validator::visit(type::unit::item::GlobalHook* g)
{
}

void Validator::visit(type::unit::item::Property* p)
{
}

void Validator::visit(type::unit::item::Variable* v)
{
}

void Validator::visit(type::unit::item::field::Constant* c)
{
}

void Validator::visit(type::unit::item::field::RegExp* r)
{
}

void Validator::visit(type::unit::item::field::Switch* s)
{
}

void Validator::visit(type::unit::item::field::Type* t)
{
}

void Validator::visit(type::unit::item::field::switch_::Case* c)
{
}

void Validator::visit(variable::Global* g)
{
}

void Validator::visit(variable::Local* l)
{
}

