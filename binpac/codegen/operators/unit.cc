
#include "cg-operator-common.h"
#include "../../attribute.h"
#include <binpac/autogen/operators/unit.h>

using namespace binpac;
using namespace binpac::codegen;

void CodeBuilder::visit(ctor::Unit* m)
{
    auto unit = ast::checkedCast<type::Unit>(m->type());
    auto func = cg()->hiltiFunctionNew(unit);
    auto hunit = cg()->hiltiTypeParseObject(unit);

    auto result = cg()->builder()->addTmp("obj", ::hilti::builder::reference::type(hunit));

    std::list<shared_ptr<hilti::Expression>> hparams;
    hparams.push_back(hilti::builder::reference::createNull());
    hparams.push_back(hilti::builder::reference::createNull());
    hparams.push_back(cg()->hiltiCookie());

    cg()->builder()->addInstruction(result, hilti::instruction::flow::CallResult, func, hilti::builder::tuple::create(hparams));

    auto hfields = ast::checkedCast<hilti::type::Struct>(hunit)->fields();
    auto vars = unit->variables();
    auto hfield = hfields.begin();
    auto var = vars.begin();

    for ( auto i : m->items() ) {
        auto id = (*hfield++)->id();
        auto init = i.second;
        auto itype = (*var++)->type();

        cg()->builder()->beginTryCatch();

        auto item = unit->item(id->name());
        auto val = hiltiExpression(init, itype);
        cg()->hiltiItemSet(result, item, val);

        cg()->builder()->pushCatch(hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::AttributeNotSet")),
                                   hilti::builder::id::node("e"));
        // Nothing to do in catch.
        cg()->builder()->popCatch();

        cg()->builder()->endTryCatch();
    }

    // Ensure we emit the unit type itself.
    cg()->hiltiAddType(unit->id(), hunit, unit);

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::unit::Attribute* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto ival = cg()->hiltiItemGet(cg()->hiltiExpression(i->op1()), item);
    setResult(ival);
}

void CodeBuilder::visit(expression::operator_::unit::AttributeAssign* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());
    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto uval = cg()->hiltiExpression(i->op1());
    auto expr = cg()->hiltiExpression(i->op3(), item->fieldType());

    auto ival = cg()->builder()->addTmp("item", cg()->hiltiType(item->fieldType()), nullptr, false);
    cg()->hiltiItemSet(uval, item, expr);

    cg()->hiltiRunFieldHooks(unit, item, uval, false, cg()->hiltiCookie());

    setResult(expr);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::HasAttribute* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto has = cg()->hiltiItemIsSet(cg()->hiltiExpression(i->op1()), item);
    setResult(has);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::TryAttribute* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    // If there's a default, the attribute access will always succeed.
    bool have_default = item->attributes()->has("default");

    if ( have_default ) {
        auto ival = cg()->hiltiItemGet(cg()->hiltiExpression(i->op1()), item);
        setResult(ival);
        return;
    }

    auto has = cg()->hiltiItemIsSet(cg()->hiltiExpression(i->op1()), item);

    auto branches = cg()->builder()->addIfElse(has);
    auto attr_set = std::get<0>(branches);
    auto attr_not_set = std::get<1>(branches);
    auto done = std::get<2>(branches);

    cg()->moduleBuilder()->pushBuilder(attr_set);
    auto ival = cg()->hiltiItemGet(cg()->hiltiExpression(i->op1()), item);
    setResult(ival);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());                                    ;
    cg()->moduleBuilder()->popBuilder(attr_set);

    cg()->moduleBuilder()->pushBuilder(attr_not_set);
    cg()->builder()->addThrow("BinPACHilti::AttributeNotSet", hilti::builder::string::create(attr->id()->name()));
    cg()->moduleBuilder()->popBuilder(attr_not_set);

    cg()->moduleBuilder()->pushBuilder(done);
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
    auto result = cg()->hiltiItemGet(self, "__input", hilti::builder::iterator::typeBytes());
    setResult(result);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Offset* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto input = cg()->hiltiItemGet(self, "__input", hilti::builder::iterator::typeBytes());
    auto cur = cg()->hiltiItemGet(self, "__cur", hilti::builder::iterator::typeBytes());

    auto result = cg()->moduleBuilder()->addTmp("offset", cg()->hiltiType(i->type()));
    cg()->builder()->addInstruction(result, hilti::instruction::bytes::Diff, input, cur);
    setResult(result);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::SetPosition* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto op1 = cg()->hiltiExpression(i->op1());

    auto tuple = ast::checkedCast<expression::Constant>(i->op3())->constant();
    auto params = ast::checkedCast<constant::Tuple>(tuple)->value();

    auto param1 = cg()->hiltiExpression(*params.begin());

    auto old = cg()->hiltiItemGet(self, "__cur", hilti::builder::iterator::typeBytes());
    cg()->hiltiItemSet(self, "__cur", param1);

    setResult(old);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::AddFilter* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto filter = callParameter(i->op3(), 0);

    // TODO: This cast here isn't ideal, better way?
    auto filter_type = cg()->hiltiCastEnum(cg()->hiltiExpression(filter), hilti::builder::type::byName("BinPACHilti::Filter"));

    auto ft = hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::ParseFilter"));
    auto head = cg()->hiltiItemGet(self, "__filter", ft, hilti::builder::reference::createNull());

    cg()->builder()->addInstruction(head,
                                    hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPACHilti::filter_add"),
                                    hilti::builder::tuple::create( { head, filter_type } ));

    cg()->hiltiItemSet(self, "__filter", head);

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Disconnect* i)
{
    auto self = cg()->hiltiExpression(i->op1());

    auto ft = hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Sink"));
    auto sink = cg()->hiltiItemGet(self, "__sink", ft, hilti::builder::reference::createNull());

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_disconnect"),
                                    hilti::builder::tuple::create( { sink, self } ));

    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::unit::MimeType* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto ft = hilti::builder::reference::type(hilti::builder::bytes::type());
    auto mt = cg()->hiltiItemGet(self, "__mimetype", ft);
    setResult(mt);
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Backtrack* i)
{
    cg()->builder()->addThrow("BinPACHilti::Backtrack", ::hilti::builder::string::create("backtracking triggered"));
    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Confirm* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto unit = ast::checkedCast<binpac::type::Unit>(i->op1()->type());
    cg()->hiltiConfirm(self, unit);
    setResult(std::make_shared<hilti::expression::Void>());
}

void CodeBuilder::visit(binpac::expression::operator_::unit::Disable* i)
{
    auto self = cg()->hiltiExpression(i->op1());
    auto unit = ast::checkedCast<binpac::type::Unit>(i->op1()->type());
    auto msg = cg()->hiltiExpression(callParameter(i->op3(), 0));
    cg()->hiltiDisable(self, unit, msg);
    setResult(std::make_shared<hilti::expression::Void>());
}

