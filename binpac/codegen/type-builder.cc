
#include "type-builder.h"
#include "type.h"

using namespace binpac;
using namespace binpac::codegen;

TypeBuilder::TypeBuilder(CodeGen* cg)
    : CGVisitor<shared_ptr<hilti::Type>>(cg, "TypeBuilder")
{
}

TypeBuilder::~TypeBuilder()
{
}

shared_ptr<hilti::Type> TypeBuilder::hiltiType(shared_ptr<Type> type)
{
    shared_ptr<hilti::Type> result;
    bool success = processOne(type, &result);
    assert(success);

    return result;
}

void TypeBuilder::visit(type::Address* a)
{
}

void TypeBuilder::visit(type::Any* a)
{
}

void TypeBuilder::visit(type::Bitset* b)
{
}

void TypeBuilder::visit(type::Block* b)
{
}

void TypeBuilder::visit(type::Bool* b)
{
}

void TypeBuilder::visit(type::Bytes* b)
{
}

void TypeBuilder::visit(type::CAddr* c)
{
}

void TypeBuilder::visit(type::Double* d)
{
}

void TypeBuilder::visit(type::Enum* e)
{
}

void TypeBuilder::visit(type::Exception* e)
{
}

void TypeBuilder::visit(type::File* f)
{
}

void TypeBuilder::visit(type::Function* f)
{
}

void TypeBuilder::visit(type::Hook* h)
{
}

void TypeBuilder::visit(type::Integer* i)
{
}

void TypeBuilder::visit(type::Interval* i)
{
}

void TypeBuilder::visit(type::Iterator* i)
{
}

void TypeBuilder::visit(type::List* l)
{
}

void TypeBuilder::visit(type::Map* m)
{
}

void TypeBuilder::visit(type::Module* m)
{
}

void TypeBuilder::visit(type::Network* n)
{
}

void TypeBuilder::visit(type::OptionalArgument* o)
{
}

void TypeBuilder::visit(type::Port* p)
{
}

void TypeBuilder::visit(type::RegExp* r)
{
}

void TypeBuilder::visit(type::Set* s)
{
}

void TypeBuilder::visit(type::String* s)
{
}

void TypeBuilder::visit(type::Time* t)
{
}

void TypeBuilder::visit(type::Tuple* t)
{
}

void TypeBuilder::visit(type::TypeByName* t)
{
}

void TypeBuilder::visit(type::TypeType* t)
{
}

void TypeBuilder::visit(type::Unit* u)
{
}

void TypeBuilder::visit(type::Unknown* u)
{
}

void TypeBuilder::visit(type::Unset* u)
{
}

void TypeBuilder::visit(type::Vector* v)
{
}

void TypeBuilder::visit(type::Void* v)
{
}

void TypeBuilder::visit(type::function::Parameter* p)
{
}

void TypeBuilder::visit(type::function::Result* r)
{
}

void TypeBuilder::visit(type::iterator::Bytes* b)
{
}

void TypeBuilder::visit(type::iterator::List* l)
{
}

void TypeBuilder::visit(type::iterator::Regexp* r)
{
}

void TypeBuilder::visit(type::iterator::Set* s)
{
}

void TypeBuilder::visit(type::iterator::Vector* v)
{
}

void TypeBuilder::visit(type::unit::Item* i)
{
}

void TypeBuilder::visit(type::unit::item::Field* f)
{
}

void TypeBuilder::visit(type::unit::item::GlobalHook* g)
{
}

void TypeBuilder::visit(type::unit::item::Property* p)
{
}

void TypeBuilder::visit(type::unit::item::Variable* v)
{
}

void TypeBuilder::visit(type::unit::item::field::Constant* c)
{
}

void TypeBuilder::visit(type::unit::item::field::RegExp* r)
{
}

void TypeBuilder::visit(type::unit::item::field::Switch* s)
{
}

void TypeBuilder::visit(type::unit::item::field::Type* t)
{
}

void TypeBuilder::visit(type::unit::item::field::switch_::Case* c)
{
}
