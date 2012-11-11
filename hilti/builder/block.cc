
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

shared_ptr<hilti::Expression> BlockBuilder::block() const
{
    assert(_block_stmt->id() && _block_stmt->id()->name().size());
    return builder::label::create(_block_stmt->id()->name());
}

shared_ptr<hilti::statement::Block> BlockBuilder::statement() const
{
    return _block_stmt;
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
    auto cont = _mbuilder->newBuilder("if-cont", l);

    _mbuilder->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, true_->block(), cont->block(), l);

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
                            shared_ptr<hilti::Expression> arg3,
                            shared_ptr<hilti::Expression> arg4
                           )
{
    hilti::builder::tuple::element_list elems;

    if ( arg1 )
        elems.push_back(arg1);

    if ( arg2 )
        elems.push_back(arg2);

    if ( arg3 )
        elems.push_back(arg3);

    if ( arg4 )
        elems.push_back(arg4);

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

void BlockBuilder::addInternalError(const std::string& msg)
{
    _mbuilder->builder()->addInstruction(hilti::instruction::debug::InternalError, builder::string::create(msg));
}

void BlockBuilder::addThrow(const std::string& ename, shared_ptr<hilti::Expression> arg)
{
    auto etype = builder::type::byName(ename);
    auto excpt = _mbuilder->addTmp("excpt", builder::reference::type(etype), nullptr);

    if ( ! arg )
        _mbuilder->builder()->addInstruction(excpt, ::hilti::instruction::exception::New, hilti::builder::type::create(etype));
    else
        _mbuilder->builder()->addInstruction(excpt, ::hilti::instruction::exception::NewWithArg, hilti::builder::type::create(etype), arg);

    _mbuilder->builder()->addInstruction(::hilti::instruction::exception::Throw, excpt);
}

void BlockBuilder::addSwitch(shared_ptr<hilti::Expression> expr,
                             shared_ptr<hilti::builder::BlockBuilder> default_,
                             const case_list& cases)
{

    std::list<shared_ptr<hilti::Expression>> tuple;

    for ( auto c : cases ) {
        std::list<shared_ptr<hilti::Expression>> pair = { c.first, c.second->block() };
        tuple.push_back(builder::tuple::create(pair));
    }

    _mbuilder->builder()->addInstruction(::hilti::instruction::flow::Switch, expr, default_->block(), builder::tuple::create(tuple));
}

void BlockBuilder::beginTryCatch()
{
    auto body = _mbuilder->pushBody(true);
    auto try_ = _mbuilder->pushBuilder("try");

    _mbuilder->_tries.push_back(std::make_shared<ModuleBuilder::TryCatch>());
    assert(_mbuilder->_tries.size());
}

void BlockBuilder::endTryCatch()
{
    assert(_mbuilder->_tries.size());

    _mbuilder->popBuilder();
    auto try_ = _mbuilder->popBody();

    auto cont = _mbuilder->newBuilder("try-cont");

    auto t = _mbuilder->_tries.back();
    auto stmt = builder::block::try_(try_->block(), t->catches);
    //_mbuilder->_tries.pop_back();

    _mbuilder->builder()->statement()->addStatement(stmt);
    _mbuilder->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    _mbuilder->pushBuilder(cont);
}

void BlockBuilder::pushCatch(shared_ptr<Type> type, shared_ptr<ID> id)
{
    assert(_mbuilder->_tries.size());

    _mbuilder->pushBody(true);
    _mbuilder->pushBuilder("catch");

    auto body = _mbuilder->_currentBody()->stmt;
    auto catch_ = builder::block::catch_(type, id, body, Location::None);
    _mbuilder->_tries.back()->catches.push_back(catch_);
}

void BlockBuilder::popCatch()
{
    assert(_mbuilder->_tries.size());

    _mbuilder->popBody();
}

void BlockBuilder::pushCatchAll()
{
    assert(_mbuilder->_tries.size());

    _mbuilder->pushBody(true);
    _mbuilder->pushBuilder("catch-all");

    auto body = _mbuilder->_currentBody()->stmt;
    auto catch_ = builder::block::catchAll(body, Location::None);
    _mbuilder->_tries.back()->catches.push_back(catch_);
}

void BlockBuilder::popCatchAll()
{
    assert(_mbuilder->_tries.size());

    _mbuilder->popBody();
}

}

}



