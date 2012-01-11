
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
