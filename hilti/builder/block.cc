
#include "builder/block.h"
#include "autogen/instructions.h"

namespace hilti {
namespace builder {

BlockBuilder::BlockBuilder(shared_ptr<statement::Block> block, shared_ptr<statement::Block> body, ModuleBuilder* mbuilder)
{
    _block_stmt = block;
    _body = body;
    _mbuilder = mbuilder;
}

BlockBuilder::~BlockBuilder()
{
}

ModuleBuilder* BlockBuilder::moduleBuilder() const
{
    return _mbuilder;
}

shared_ptr<hilti::expression::Block> BlockBuilder::block() const
{
    return std::make_shared<hilti::expression::Block>(_block_stmt);
}

shared_ptr<hilti::expression::Variable> BlockBuilder::addLocal(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    id = _mbuilder->uniqueID(id, _body->scope(), force_unique, false);
    auto var = std::make_shared<variable::Local>(id, type, init, l);
    auto decl = std::make_shared<declaration::Variable>(id, var, l);
    _block_stmt->addDeclaration(decl);
    return std::make_shared<hilti::expression::Variable>(var, l);
}

shared_ptr<hilti::expression::Variable> BlockBuilder::addLocal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addLocal(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::expression::Variable> BlockBuilder::addTmp(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool reuse, const Location& l)
{
    return _mbuilder->addTmp(id, type, init, reuse, l);
}

shared_ptr<hilti::expression::Variable> BlockBuilder::addTmp(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool reuse, const Location& l)
{
    return addTmp(std::make_shared<ID>(id, l), type, init, reuse, l);
}

void BlockBuilder::setBlockComment(const std::string& comment)
{
    _block_stmt->setComment(comment);
}

void BlockBuilder::createComment(const std::string& comment)
{
    _next_comments.push_back(comment);
}

void BlockBuilder::addInstruction(shared_ptr<Statement> stmt)
{
    auto comment = ::util::strjoin(_next_comments, "; ");
    _next_comments.clear();
    stmt->setComment(comment);
    _block_stmt->addStatement(stmt);
}

shared_ptr<statement::Instruction> BlockBuilder::addInstruction(shared_ptr<Instruction> ins,
                                                                shared_ptr<Expression> op1,
                                                                shared_ptr<Expression> op2,
                                                                shared_ptr<Expression> op3,
                                                                const Location& l)
{
    return addInstruction(nullptr, ins, op1, op2, op3);
}

shared_ptr<statement::Instruction> BlockBuilder::addInstruction(shared_ptr<Expression> target,
                                                                shared_ptr<Instruction> ins,
                                                                shared_ptr<Expression> op1,
                                                                shared_ptr<Expression> op2,
                                                                shared_ptr<Expression> op3,
                                                                const Location& l)
{
    hilti::instruction::Operands ops = { target, op1, op2, op3 };
    auto instr = std::make_shared<statement::instruction::Unresolved>(ins, ops, l);
    addInstruction(instr);
    return instr;
}

std::tuple<shared_ptr<BlockBuilder>, shared_ptr<BlockBuilder>> BlockBuilder::addIf(shared_ptr<Expression> cond, const Location& l)
{
    auto true_ = _mbuilder->newBuilder("if-true", l);
    auto false_ = _mbuilder->newBuilder("if-false", l);
    auto cont = _mbuilder->newBuilder("if-cont", l);

    //_mbuilder->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, true_, false_, l);
    _mbuilder->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, true_->block(), false_->block(), l);

    false_->addInstruction(hilti::instruction::flow::Jump, cont->block(), nullptr, nullptr, l);

    return std::make_tuple(true_, cont);
}

std::tuple<shared_ptr<BlockBuilder>, shared_ptr<BlockBuilder>, shared_ptr<BlockBuilder>> BlockBuilder::addIfElse(shared_ptr<Expression> cond, const Location& l)
{
    auto true_ = _mbuilder->newBuilder("if-true", l);
    auto false_ = _mbuilder->newBuilder("if-false", l);
    auto cont = _mbuilder->newBuilder("if-cont", l);

    _mbuilder->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, true_->block(), false_->block(), l);

    return std::make_tuple(true_, false_, cont);
}



}

}



