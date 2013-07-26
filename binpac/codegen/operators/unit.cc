
#include "cg-operator-common.h"
#include <binpac/autogen/operators/unit.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(expression::operator_::unit::Attribute* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto ival = cg()->builder()->addTmp("item", cg()->hiltiType(item->fieldType()), nullptr, false);
    cg()->builder()->addInstruction(ival,
                                    hilti::instruction::struct_::Get,
                                    cg()->hiltiExpression(i->op1()),
                                    hilti::builder::string::create(attr->id()->name()));

    setResult(ival);
}

void CodeBuilder::visit(expression::operator_::unit::AttributeAssign* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());
    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto expr = cg()->hiltiExpression(i->op3(), item->fieldType());

    auto ival = cg()->builder()->addTmp("item", cg()->hiltiType(item->fieldType()), nullptr, false);
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    cg()->hiltiExpression(i->op1()),
                                    hilti::builder::string::create(attr->id()->name()),
                                    expr);

    cg()->hiltiRunFieldHooks(item);

    setResult(expr);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::HasAttribute* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto has = cg()->builder()->addTmp("has", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(has,
                                    hilti::instruction::struct_::IsSet,
                                    cg()->hiltiExpression(i->op1()),
                                    hilti::builder::string::create(attr->id()->name()));

    setResult(has);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::New* i)
{
    auto ttype = ast::checkedCast<type::TypeType>(i->op1()->type());
    auto unit = ast::checkedCast<type::Unit>(ttype->typeType());
    auto params = i->op2() ? callParameters(i->op2()) : expression_list();

    auto func = cg()->hiltiFunctionNew(unit);

    std::list<shared_ptr<hilti::Expression>> hparams;

    for ( auto p : params )
        hparams.push_back(cg()->hiltiExpression(p));

    hparams.push_back(hilti::builder::reference::createNull());
    hparams.push_back(hilti::builder::reference::createNull());
    hparams.push_back(cg()->hiltiCookie());

    auto result = cg()->builder()->addTmp("pobj", cg()->hiltiType(unit));
    cg()->builder()->addInstruction(result, hilti::instruction::flow::CallResult, func, hilti::builder::tuple::create(hparams));

    setResult(result);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Input* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto result = cg()->moduleBuilder()->addTmp("input", hilti::builder::iterator::typeBytes());
    cg()->builder()->addInstruction(result, hilti::instruction::struct_::Get, self, hilti::builder::string::create("__input"));
    setResult(result);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Offset* i)
{
    auto self = cg()->hiltiExpression(i->op1());

    auto result = cg()->moduleBuilder()->addTmp("offset", hilti::builder::integer::type(64));
    auto input = cg()->moduleBuilder()->addTmp("input", hilti::builder::iterator::typeBytes());
    auto cur = cg()->moduleBuilder()->addTmp("cur", hilti::builder::iterator::typeBytes());

    cg()->builder()->addInstruction(input, hilti::instruction::struct_::Get, self, hilti::builder::string::create("__input"));
    cg()->builder()->addInstruction(cur, hilti::instruction::struct_::Get, self, hilti::builder::string::create("__cur"));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Diff, input, cur);

    setResult(result);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::SetPosition* i)
{
    auto self = cg()->hiltiExpression(i->op1());

    auto old = cg()->moduleBuilder()->addTmp("old_cur", hilti::builder::iterator::typeBytes());
    auto op1 = cg()->hiltiExpression(i->op1());

    auto tuple = ast::checkedCast<expression::Constant>(i->op3())->constant();
    auto params = ast::checkedCast<constant::Tuple>(tuple)->value();

    auto param1 = cg()->hiltiExpression(*params.begin());

    cg()->builder()->addInstruction(old, hilti::instruction::struct_::Get, self, hilti::builder::string::create("__cur"));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, self, hilti::builder::string::create("__cur"), param1);

    setResult(old);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::AddFilter* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto filter = callParameter(i->op3(), 0);

    // TODO: This cast here isn't ideal, better way?
    auto filter_type = cg()->hiltiCastEnum(cg()->hiltiExpression(filter), hilti::builder::type::byName("BinPACHilti::Filter"));

    auto head = cg()->builder()->addTmp("filter", hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::ParseFilter")));

    cg()->builder()->addInstruction(head,
                                    hilti::instruction::struct_::GetDefault,
                                    self,
                                    hilti::builder::string::create("__filter"),
                                    hilti::builder::reference::createNull());

    cg()->builder()->addInstruction(head,
                                    hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPACHilti::filter_add"),
                                    hilti::builder::tuple::create( { head, filter_type } ));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, self, hilti::builder::string::create("__filter"), head);

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Disconnect* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto sink = cg()->builder()->addTmp("sink", cg()->hiltiType(std::make_shared<type::Sink>()));

    cg()->builder()->addInstruction(sink,
                                    hilti::instruction::struct_::GetDefault,
                                    self,
                                    hilti::builder::string::create("__sink"),
                                    hilti::builder::reference::createNull());

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_disconnect"),
                                    hilti::builder::tuple::create( { sink, self } ));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::unit::MimeType* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto mt = cg()->builder()->addLocal("mt", hilti::builder::reference::type(hilti::builder::bytes::type()));

    cg()->builder()->addInstruction(mt,
                                    hilti::instruction::struct_::Get,
                                    self,
                                    hilti::builder::string::create("__mimetype"));

    setResult(mt);
}
