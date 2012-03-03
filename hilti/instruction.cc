
#include "instruction.h"
#include "statement.h"
#include "variable.h"
#include "module.h"

#include "passes/validator.h"

using namespace hilti;

const int _num_ops = 4; // Number of operands we support, including target.

auto hilti::theInstructionRegistry = unique_ptr<InstructionRegistry>(new InstructionRegistry);


shared_ptr<Type> InstructionHelper::typedType(shared_ptr<Expression> op) const
{
    auto t = ast::as<type::Type>(op);
    assert(t);
    return t->typeType();
}

shared_ptr<Type> InstructionHelper::referencedType(shared_ptr<Expression> op) const
{
    auto t = ast::as<type::Reference>(op);
    assert(t);
    return t->argType();
}

shared_ptr<Type> InstructionHelper::elementType(shared_ptr<Expression> op) const
{
    return elementType(op->type());
}

shared_ptr<Type> InstructionHelper::elementType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t = ast::as<type::TypedHeapType>(ty);
    assert(t);

    return t->argType();
}

shared_ptr<Type> InstructionHelper::iteratedType(shared_ptr<Expression> op) const
{
    auto t = ast::as<type::Iterator>(op);
    assert(t);
    return t->argType();
}

shared_ptr<Type> InstructionHelper::mapKeyType(shared_ptr<Expression> op) const
{
    return mapKeyType(op->type());
}

shared_ptr<Type> InstructionHelper::mapKeyType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t = ast::as<type::Map>(ty);
    assert(t);

    return t->keyType();
}

shared_ptr<Type> InstructionHelper::mapValueType(shared_ptr<Expression> op) const
{
    return mapValueType(op->type());
}

shared_ptr<Type> InstructionHelper::mapValueType(shared_ptr<Type> ty) const
{
    auto r = ast::as<type::Reference>(ty);

    if ( r )
        ty = r->argType();

    auto t = ast::as<type::Map>(ty);
    assert(t);

    return t->valueType();
}

Instruction::operator string() const
{
    std::list<string> ops;

    if ( __typeOp1() )
        ops.push_back(__typeOp1()->render());

    if ( __typeOp2() )
        ops.push_back(__typeOp2()->render());

    if ( __typeOp3() )
        ops.push_back(__typeOp3()->render());

    string s = _id->name() + ": " + util::strjoin(ops, ", ");

    if ( __typeOp0() )
        s += " -> " + __typeOp0()->render();

    return s;
}

void InstructionRegistry::addInstruction(shared_ptr<Instruction> ins)
{
    _instructions.insert(instr_map::value_type(ins->id()->name(), ins));
}

InstructionRegistry::instr_list InstructionRegistry::getMatching(shared_ptr<ID> id, const instruction::Operands& ops) const
{
    instr_list matches;

    auto m = _instructions.equal_range(id->name());

    for ( auto n = m.first; n != m.second; ++n ) {
        shared_ptr<Instruction> instr = n->second;

        if ( instr->matchesOperands(ops) )
            matches.push_back(instr);
    }

    return matches;
}

InstructionRegistry::instr_list InstructionRegistry::byName(const string& name) const
{
    instr_list matches;

    auto m = _instructions.equal_range(name);

    for ( auto n = m.first; n != m.second; ++n ) {
        shared_ptr<Instruction> instr = n->second;
        matches.push_back(instr);
    }

    return matches;
}

bool Instruction::matchesOperands(const instruction::Operands& ops)
{
    if ( ! __matchOp0(ops[0]) )
        return false;

    if ( ! __matchOp1(ops[1]) )
        return false;

    if ( ! __matchOp2(ops[2]) )
        return false;

    if ( ! __matchOp3(ops[3]) )
        return false;

    return true;
}

void Instruction::error(Node* op, string msg) const
{
    assert(_validator);
    _validator->error(op, msg);
}

bool Instruction::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Type> target) const
{
    if ( op->canCoerceTo(target) )
        return true;

    error(op, util::fmt("operand type %s is not compatible with type %s",
                        op->type()->render().c_str(), target->render().c_str()));
    return false;
}

bool Instruction::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Expression> target) const
{
    if ( op->canCoerceTo(target->type()) )
        return true;

    error(op, util::fmt("operand type %s is not compatible with target type %s",
                        op->type()->render().c_str(), target->type()->render().c_str()));
    return false;
}

bool Instruction::canCoerceTo(shared_ptr<Type> src, shared_ptr<Expression> target) const
{
    if ( Coercer().canCoerceTo(src, target->type()) )
        return true;

    error(target, util::fmt("type %s is not compatible with target type %s",
                        src->render().c_str(), target->type()->render().c_str()));
    return false;
}

bool Instruction::canCoerceTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const
{
   if ( op1->canCoerceTo(op2->type()) || op2->canCoerceTo(op1->type()) )
        return true;

    error(op1, util::fmt("operand types %s and %s are not compatible",
                        op1->type()->render().c_str(), op2->type()->render().c_str()));

    return false;
}

bool Instruction::hasType(shared_ptr<Expression> op, shared_ptr<Type> target) const
{
    if ( op->type() == target )
        return true;

    error(op, util::fmt("operand type %s does not match expected type %s",
                        op->type()->render().c_str(), target->render().c_str()));

    return false;
}

bool Instruction::equalTypes(shared_ptr<Type> ty1, shared_ptr<Type> ty2) const
{
    if ( ty1 == ty2 )
        return true;

    error(ty1, util::fmt("types %s and %s do not match",
                        ty1->render().c_str(), ty2->render().c_str()));

    return false;
}

bool Instruction::equalTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const
{
    if ( op1->type() == op2->type() )
        return true;

    error(op1, util::fmt("operand types %s and %s do not match",
                        op1->type()->render().c_str(), op2->type()->render().c_str()));

    return false;
}

bool Instruction::isConstant(shared_ptr<Expression> op) const
{
    if ( ast::as<expression::Constant>(op) )
        return true;

    error(op, "operand must be a constant");

    return false;
}

shared_ptr<statement::instruction::Resolved> InstructionRegistry::resolveStatement(shared_ptr<Instruction> instr, shared_ptr<statement::Instruction> stmt)
{
    instruction::Operands new_ops;
    instruction::Operands defaults;

    defaults.resize(_num_ops);
    defaults[0] = node_ptr<Expression>();
    defaults[1] = instr->__defaultOp1();
    defaults[2] = instr->__defaultOp2();
    defaults[3] = instr->__defaultOp3();

    for ( int i = 0; i < _num_ops; i++ ) {
        auto op = stmt->operands()[i];
        new_ops.push_back(op ? op : defaults[i]);
    }

    return (*instr->factory())(instr, new_ops, stmt->location());
}

