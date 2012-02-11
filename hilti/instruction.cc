
#include "instruction.h"
#include "statement.h"
#include "variable.h"
#include "module.h"

#include "passes/validator.h"

using namespace hilti;

const int _num_ops = 4; // Number of operands we support, including target.

auto hilti::theInstructionRegistry = unique_ptr<InstructionRegistry>(new InstructionRegistry);

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

shared_ptr<Instruction> InstructionRegistry::byName(const string& name) const
{
    instr_list matches;

    auto m = _instructions.equal_range(name);

    for ( auto n = m.first; n != m.second; ++n ) {
        shared_ptr<Instruction> instr = n->second;
        matches.push_back(instr);
    }

    assert(matches.size() == 1);
    return matches.front();
}

bool Instruction::matchesOperands(const instruction::Operands& ops) const
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

    error(op, util::fmt("operand type %s not compatible with type %s",
                        op->type()->repr().c_str(), target->repr().c_str()));
    return false;
}

bool Instruction::canCoerceTo(shared_ptr<Expression> op, shared_ptr<Expression> target) const
{
    if ( op->canCoerceTo(target->type()) )
        return true;

    error(op, util::fmt("operand type %s not compatible with target type %s",
                        op->type()->repr().c_str(), target->type()->repr().c_str()));
    return false;
}

bool Instruction::canCoerceTypes(shared_ptr<Expression> op1, shared_ptr<Expression> op2) const
{
   if ( op1->canCoerceTo(op2->type()) || op2->canCoerceTo(op1->type()) )
        return true;

    error(op1, util::fmt("operand types %s and %s are not compatible",
                        op1->type()->repr().c_str(), op2->type()->repr().c_str()));

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
