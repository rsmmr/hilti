
#include "hilti.h"

using namespace hilti::passes;

Validator::~Validator()
{
}

void Validator::visit(Module* m)
{
}

void Validator::visit(statement::Block* b)
{
}

void Validator::visit(statement::instruction::Unresolved* i)
{
    error(i, "unresolved instruction (should not happen, probably an internal error)");
}

void Validator::visit(statement::instruction::Resolved* i)
{
    i->instruction()->validate(this, i->operands());
}

void Validator::visit(expression::Constant* e)
{
}

void Validator::visit(declaration::Variable* v)
{
}

void Validator::visit(ID* i)
{
}

void Validator::visit(type::Integer* i)
{
}

void Validator::visit(type::String* i)
{
}

void Validator::visit(constant::Integer* i)
{
}

void Validator::visit(constant::String* s)
{
}
