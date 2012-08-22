
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
    return _mbuilder->_addLocal(_block_stmt, id, type, init, force_unique, l);
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

void BlockBuilder::addBlockComment(const std::string& comment)
{
    _block_stmt->addComment(util::strtrim(comment));
}

void BlockBuilder::addComment(const std::string& comment)
{
    _next_comments.push_back(util::strtrim(comment));
}

void BlockBuilder::addInstruction(shared_ptr<Statement> stmt)
{
    for ( auto c : _next_comments )
        stmt->addComment(c);

    _next_comments.clear();
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

void BlockBuilder::addDebugMsg(const std::string& stream, const std::string& msg)
{
    if ( msg.find("%") == std::string::npos )
        addDebugMsg(stream, msg, nullptr);
    else
        addDebugMsg(stream, "%s", hilti::builder::string::create(msg));
}

void BlockBuilder::addDebugMsg(const std::string& stream, const std::string& msg,
                            shared_ptr<hilti::Expression> arg1,
                            shared_ptr<hilti::Expression> arg2,
                            shared_ptr<hilti::Expression> arg3
                           )
{
    hilti::builder::tuple::element_list elems;

    if ( arg1 )
        elems.push_back(arg1);

    if ( arg2 )
        elems.push_back(arg2);

    if ( arg3 )
        elems.push_back(arg3);

    auto t = hilti::builder::tuple::create(elems);

    _mbuilder->builder()->addInstruction(hilti::instruction::debug::Msg,
                                         hilti::builder::string::create(stream),
                                         hilti::builder::string::create(msg),
                                         t);
}

void BlockBuilder::debugPushIndent()
{
    _mbuilder->builder()->addInstruction(hilti::instruction::debug::PushIndent);
}

void BlockBuilder::debugPopIndent()
{
    _mbuilder->builder()->addInstruction(hilti::instruction::debug::PopIndent);
}

}

}



