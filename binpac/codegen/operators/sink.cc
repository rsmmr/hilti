
#include "cg-operator-common.h"
#include <binpac/autogen/operators/sink.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(binpac::expression::operator_::sink::Close* i)
{
    auto sink = cg()->hiltiExpression(i->op1());

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_close"),
                                    hilti::builder::tuple::create( { sink } ));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::sink::Connect* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto unit = cg()->hiltiExpression(callParameter(i->op3(), 0));

    // Check that it's not already connected.
    auto old_sink = cg()->builder()->addTmp("old_sink", cg()->hiltiType(std::make_shared<type::Sink>()));

    cg()->builder()->addInstruction(old_sink,
                                    hilti::instruction::struct_::GetDefault,
                                    unit,
                                    hilti::builder::string::create("__sink"),
                                    hilti::builder::reference::createNull());

    auto branches = cg()->builder()->addIf(old_sink);
    auto sink_set = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(sink_set);
    cg()->builder()->addThrow("BinPACHilti::UnitAlreadyConnected");
    cg()->moduleBuilder()->popBuilder(sink_set);

    // Connect it.

    cg()->moduleBuilder()->pushBuilder(cont);

    auto parser = cg()->builder()->addTmp("parser", hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Parser")));

    cg()->builder()->addInstruction(parser,
                                    hilti::instruction::struct_::Get,
                                    unit,
                                    hilti::builder::string::create("__parser"));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_connect"),
                                    hilti::builder::tuple::create( { sink, unit, parser } ));

    // Set the __sink field.
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    unit,
                                    hilti::builder::string::create("__sink"),
                                    sink);

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::sink::ConnectMimeTypeString* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto mtype = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_connect_mimetype_string"),
                                    hilti::builder::tuple::create( { sink, mtype, cg()->hiltiCookie() } ));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::sink::ConnectMimeTypeBytes* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto mtype = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_connect_mimetype_bytes"),
                                    hilti::builder::tuple::create( { sink, mtype, cg()->hiltiCookie() } ));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::sink::Write* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto data = cg()->hiltiExpression(callParameter(i->op3(), 0));

    cg()->hiltiWriteToSink(sink, data);

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::sink::AddFilter* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto filter = cg()->hiltiExpression(callParameter(i->op3(), 0));

    // TODO: This cast here isn't ideal, better way?
    auto filter_type = cg()->hiltiCastEnum(filter, hilti::builder::type::byName("BinPACHilti::Filter"));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_add_filter"),
                                    hilti::builder::tuple::create( { sink, filter_type } ));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(expression::operator_::sink::Size* i)
{
    auto sink = cg()->hiltiExpression(i->op1());
    auto size = cg()->builder()->addTmp("size", hilti::builder::integer::type(64));

    cg()->builder()->addInstruction(size,
                                    hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPACHilti::sink_size"),
                                    hilti::builder::tuple::create( { sink } ));
    setResult(size);
}
