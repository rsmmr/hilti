
#include "cg-operator-common.h"
#include <binpac/autogen/operators/bytes.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(ctor::Bytes* b)
{
    auto result = hilti::builder::bytes::create(b->value(), b->location());
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Plus* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());

    auto copy =
        cg()->builder()->addTmp("copy",
                                hilti::builder::reference::type(hilti::builder::bytes::type()));
    cg()->builder()->addInstruction(copy, hilti::instruction::operator_::Clone, op1);
    cg()->builder()->addInstruction(hilti::instruction::bytes::Append, copy, op2);
    setResult(copy);
}

void CodeBuilder::visit(expression::operator_::bytes::PlusAssign* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(hilti::instruction::bytes::Append, op1, op2);
    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::bytes::Size* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto size = cg()->builder()->addTmp("item", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(size, hilti::instruction::bytes::Length, op1);
    setResult(size);
}

void CodeBuilder::visit(expression::operator_::bytes::Match* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto re = cg()->hiltiExpression(callParameter(i->op3(), 0));

    auto g = callParameter(i->op3(), 1);
    auto group = g ? cg()->hiltiExpression(g) : hilti::builder::integer::create(0);

    auto result =
        cg()->builder()->addTmp("match",
                                hilti::builder::reference::type(hilti::builder::bytes::type()));

    cg()->builder()->addInstruction(result, hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPACHilti::bytes_match"),
                                    hilti::builder::tuple::create({op1, re, group}));

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Begin* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto iter = cg()->builder()->addTmp("iter", hilti::builder::iterator::typeBytes());
    cg()->builder()->addInstruction(iter, hilti::instruction::operator_::Begin, op1);
    setResult(iter);
}

void CodeBuilder::visit(expression::operator_::bytes::End* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto iter = cg()->builder()->addTmp("iter", hilti::builder::iterator::typeBytes());
    cg()->builder()->addInstruction(iter, hilti::instruction::operator_::End, op1);
    setResult(iter);
}

void CodeBuilder::visit(expression::operator_::bytes::Upper* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto result =
        cg()->builder()->addTmp("result",
                                hilti::builder::reference::type(hilti::builder::bytes::type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Upper, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Lower* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto result =
        cg()->builder()->addTmp("result",
                                hilti::builder::reference::type(hilti::builder::bytes::type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Lower, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Strip* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());

    shared_ptr<hilti::Expression> op2 = nullptr;

    auto side = callParameter(i->op3(), 0);

    if ( side ) {
        auto t1 = hilti::builder::tuple::create(
            {hilti::builder::id::create("BinPAC::Side::Left"),
             hilti::builder::id::create(string("Hilti::Side::Left"))});

        auto t2 = hilti::builder::tuple::create(
            {hilti::builder::id::create("BinPAC::Side::Right"),
             hilti::builder::id::create(string("Hilti::Side::Right"))});

        auto t3 = hilti::builder::tuple::create(
            {hilti::builder::id::create("BinPAC::Side::Both"),
             hilti::builder::id::create(string("Hilti::Side::Both"))});

        op2 = moduleBuilder()->addTmp("side", hilti::builder::type::byName("Hilti::Side"));
        auto tuple = hilti::builder::tuple::create({t1, t2, t3});
        auto hside = cg()->hiltiExpression(side);

        cg()->builder()->addInstruction(op2, hilti::instruction::Misc::SelectValue, hside, tuple);
    }

    auto chars = callParameter(i->op3(), 1);
    auto op3 = chars ? cg()->hiltiExpression(chars) : nullptr;

    auto result =
        cg()->builder()->addTmp("result",
                                hilti::builder::reference::type(hilti::builder::bytes::type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Strip, op1, op2, op3);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Split1* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto sep = callParameter(i->op3(), 0);
    auto hsep = sep ? cg()->hiltiExpression(sep) : hilti::builder::bytes::create("");

    auto tb = hilti::builder::reference::type(hilti::builder::bytes::type());
    auto tt = hilti::builder::tuple::type({tb, tb});
    auto result = cg()->builder()->addTmp("result", tt);
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Split1, op1, hsep);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Split* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto sep = callParameter(i->op3(), 0);
    auto hsep = sep ? cg()->hiltiExpression(sep) : hilti::builder::bytes::create("");

    auto tb = hilti::builder::reference::type(hilti::builder::bytes::type());
    auto tv = hilti::builder::reference::type(hilti::builder::vector::type(tb));
    auto result = cg()->builder()->addTmp("result", tv);
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Split, op1, hsep);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::StartsWith* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(callParameter(i->op3(), 0));
    auto result = cg()->builder()->addTmp("starts", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::StartsWith, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::ToInt* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto base = callParameter(i->op3(), 0);
    auto hbase = base ? cg()->hiltiExpression(base) : hilti::builder::integer::create(10);

    auto result = cg()->builder()->addTmp("i", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::ToIntFromAscii, op1, hbase);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::ToUInt* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto base = callParameter(i->op3(), 0);
    auto hbase = base ? cg()->hiltiExpression(base) : hilti::builder::integer::create(10);

    auto result = cg()->builder()->addTmp("u", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::ToIntFromAscii, op1, hbase);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::ToIntBinary* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto order = cg()->hiltiByteOrder(callParameter(i->op3(), 0));

    auto result = cg()->builder()->addTmp("i", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::ToIntFromBinary, op1, order);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::ToUIntBinary* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto order = cg()->hiltiByteOrder(callParameter(i->op3(), 0));

    auto result = cg()->builder()->addTmp("i", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::ToUIntFromBinary, op1,
                                    order);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::ToTime* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto base = callParameter(i->op3(), 0);
    auto hbase = base ? cg()->hiltiExpression(base) : hilti::builder::integer::create(10);

    auto n = cg()->builder()->addTmp("n", hilti::builder::integer::type(64));
    cg()->builder()->addInstruction(n, hilti::instruction::bytes::ToIntFromAscii, op1, hbase);

    auto result = cg()->builder()->addTmp("result", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::integer::AsTime, n);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::ToTimeBinary* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto order = cg()->hiltiByteOrder(callParameter(i->op3(), 0));

    auto n = cg()->builder()->addTmp("n", hilti::builder::integer::type(64));
    cg()->builder()->addInstruction(n, hilti::instruction::bytes::ToIntFromBinary, op1, order);

    auto result = cg()->builder()->addTmp("result", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::integer::AsTime, n);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Decode* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto charset = cg()->hiltiCharset(callParameter(i->op3(), 0));

    auto result = cg()->builder()->addTmp("result", hilti::builder::string::type());
    cg()->builder()->addInstruction(result, hilti::instruction::string::Decode, op1, charset);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bytes::Join* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(callParameter(i->op3(), 0));

    auto result =
        cg()->builder()->addTmp("result",
                                hilti::builder::reference::type(hilti::builder::bytes::type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Join, op1, op2);

    setResult(result);
}
