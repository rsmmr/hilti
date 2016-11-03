
#include "cg-operator-common.h"
#include <spicy/autogen/operators/sink.h>

using namespace spicy;
using namespace spicy::codegen;

void CodeBuilder::visit(spicy::expression::operator_::sink::New* i)
{
    auto stype = std::make_shared<type::Sink>();
    auto result = cg()->builder()->addTmp("sink", cg()->hiltiType(stype));

    cg()->builder()->addInstruction(result, hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("SpicyHilti::sink_new"),
                                    hilti::builder::tuple::create({}));

    setResult(result);
}

void CodeBuilder::visit(spicy::expression::operator_::sink::Close* i)
{
    auto sink = cg()->hiltiExpression(i->op1());

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_close"),
                                    hilti::builder::tuple::create({sink, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::Connect* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto unit = cg()->hiltiExpression(callParameter(i->op3(), 0));

    // Check that it's not already connected.
    auto ftype = hilti::builder::reference::type(hilti::builder::type::byName("SpicyHilti::Sink"));
    auto old_sink =
        cg()->hiltiItemGet(unit, "__sink", ftype, hilti::builder::reference::createNull());

    auto branches = cg()->builder()->addIf(old_sink);
    auto sink_set = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(sink_set);
    cg()->builder()->addThrow("SpicyHilti::UnitAlreadyConnected");
    cg()->moduleBuilder()->popBuilder(sink_set);

    // Connect it.

    cg()->moduleBuilder()->pushBuilder(cont);

    auto ptype =
        hilti::builder::reference::type(hilti::builder::type::byName("SpicyHilti::Parser"));
    auto parser = cg()->hiltiItemGet(unit, "__parser", ptype);

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_connect"),
                                    hilti::builder::tuple::create({sink, unit, parser}));

    // Set the __sink field.
    cg()->hiltiItemSet(unit, "__sink", sink);

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::ConnectMimeTypeString* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto mtype = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create(
                                        "SpicyHilti::sink_connect_mimetype_string"),
                                    hilti::builder::tuple::create(
                                        {sink, mtype, hilti::builder::boolean::create(false),
                                         cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::ConnectMimeTypeBytes* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto mtype = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create(
                                        "SpicyHilti::sink_connect_mimetype_bytes"),
                                    hilti::builder::tuple::create(
                                        {sink, mtype, hilti::builder::boolean::create(false),
                                         cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::TryConnectMimeTypeString* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto mtype = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create(
                                        "SpicyHilti::sink_connect_mimetype_string"),
                                    hilti::builder::tuple::create(
                                        {sink, mtype, hilti::builder::boolean::create(true),
                                         cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::TryConnectMimeTypeBytes* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto mtype = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create(
                                        "SpicyHilti::sink_connect_mimetype_bytes"),
                                    hilti::builder::tuple::create(
                                        {sink, mtype, hilti::builder::boolean::create(true),
                                         cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::Write* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto data = cg()->hiltiExpression(callParameter(i->op3(), 0));
    auto seq = callParameter(i->op3(), 1);
    auto len = callParameter(i->op3(), 2);

    cg()->hiltiWriteToSink(sink, data, seq ? cg()->hiltiExpression(seq) : nullptr,
                           len ? cg()->hiltiExpression(len) : nullptr);

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(spicy::expression::operator_::sink::AddFilter* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto filter = cg()->hiltiExpression(callParameter(i->op3(), 0));

    // TODO: This cast here isn't ideal, better way?
    auto filter_type =
        cg()->hiltiCastEnum(filter, hilti::builder::type::byName("SpicyHilti::Filter"));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_add_filter"),
                                    hilti::builder::tuple::create({sink, filter_type}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::Size* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto size = cg()->builder()->addTmp("size", cg()->hiltiType(i->type()));

    cg()->builder()->addInstruction(size, hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("SpicyHilti::sink_size"),
                                    hilti::builder::tuple::create({sink}));
    setResult(size);
}

void CodeBuilder::visit(expression::operator_::sink::Sequence* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto size = cg()->builder()->addTmp("seq", cg()->hiltiType(i->type()));

    cg()->builder()->addInstruction(size, hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("SpicyHilti::sink_sequence"),
                                    hilti::builder::tuple::create({sink}));
    setResult(size);
}

void CodeBuilder::visit(expression::operator_::sink::Gap* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto seq = cg()->hiltiExpression(callParameter(i->op3(), 0));
    auto len = cg()->hiltiExpression(callParameter(i->op3(), 1));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_gap"),
                                    hilti::builder::tuple::create(
                                        {sink, seq, len, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::Skip* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto seq = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_skip"),
                                    hilti::builder::tuple::create(
                                        {sink, seq, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::Trim* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto seq = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_trim"),
                                    hilti::builder::tuple::create(
                                        {sink, seq, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::SetInitialSequenceNumber* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto seq = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create(
                                        "SpicyHilti::sink_set_initial_sequence_number"),
                                    hilti::builder::tuple::create(
                                        {sink, seq, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::SetAutoTrim* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto enabled = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_set_auto_trim"),
                                    hilti::builder::tuple::create(
                                        {sink, enabled, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::SetPolicy* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto policy = cg()->hiltiExpression(callParameter(i->op3(), 0));

    auto p = cg()->builder()->addTmp("p", ::hilti::builder::integer::type(64));

    cg()->builder()->addInstruction(p, hilti::instruction::enum_::ToInt, policy);

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("SpicyHilti::sink_set_policy"),
                                    hilti::builder::tuple::create({sink, p, cg()->hiltiCookie()}));

    setResult(std::make_shared<hilti::expression::Void>());
}
