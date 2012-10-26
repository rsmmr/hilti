
#include "code-builder.h"
#include "declaration.h"
#include "expression.h"
#include "type.h"
#include "statement.h"
#include "function.h"
#include "grammar.h"

#include "autogen/operators/bool.h"
#include "autogen/operators/enum.h"
#include "autogen/operators/function.h"
#include "autogen/operators/integer.h"
#include "autogen/operators/list.h"
#include "autogen/operators/tuple.h"
#include "autogen/operators/unit.h"

using namespace binpac;
using namespace binpac::codegen;

CodeBuilder::CodeBuilder(CodeGen* cg)
    : CGVisitor<shared_ptr<hilti::Node>, shared_ptr<Type>>(cg, "CodeBuilder")
{
}

CodeBuilder::~CodeBuilder()
{
}

void CodeBuilder::hiltiStatement(shared_ptr<Statement> stmt)
{
    if ( ! ast::tryCast<statement::Block>(stmt) )
        cg()->builder()->addComment(util::fmt("Statement: %s", stmt->render()));

    bool success = processOne(stmt);
    assert(success);
}

shared_ptr<hilti::Expression> CodeBuilder::hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to)
{
    if ( coerce_to )
        expr = expr->coerceTo(coerce_to);

    shared_ptr<hilti::Node> result;
    bool success = processOne(expr, &result, coerce_to);
    assert(success);

    assert(result);
    return ast::checkedCast<hilti::Expression>(result);
}

void CodeBuilder::hiltiBindDollarDollar(shared_ptr<hilti::Expression> val)
{
    _dollardollar = val;
}

void CodeBuilder::visit(Module* m)
{
    hiltiStatement(m->body());
}

void CodeBuilder::visit(Function* f)
{
}

void CodeBuilder::visit(Hook* h)
{
}

void CodeBuilder::visit(constant::Address* a)
{
}

void CodeBuilder::visit(constant::Bitset* b)
{
}

void CodeBuilder::visit(constant::Bool* b)
{
    auto result = hilti::builder::boolean::create(b->value());
    setResult(result);
}

void CodeBuilder::visit(constant::Double* d)
{
}

void CodeBuilder::visit(constant::Enum* e)
{
    assert(e->type()->id());

    auto expr =  e->firstParent<Expression>();
    assert(expr);

    ID::component_list path = { expr->scope(), e->value()->name() };
    auto fq = std::make_shared<ID>(path, e->location());
    auto result = hilti::builder::id::create(cg()->hiltiID(fq));
    setResult(result);
}

#if 0
void CodeBuilder::visit(constant::Expression* e)
{
}
#endif

void CodeBuilder::visit(constant::Integer* i)
{
    auto result = hilti::builder::integer::create(i->value(), i->location());
    setResult(result);
}

void CodeBuilder::visit(constant::Interval* i)
{
}

void CodeBuilder::visit(constant::Network* n)
{
}

void CodeBuilder::visit(constant::Port* p)
{
}

void CodeBuilder::visit(constant::String* s)
{
    auto result = hilti::builder::string::create(s->value(), s->location());
    setResult(result);
}

void CodeBuilder::visit(constant::Time* t)
{
}

void CodeBuilder::visit(constant::Tuple* t)
{
    hilti::builder::tuple::element_list elems;

    for ( auto e : t->value() )
        elems.push_back(cg()->hiltiExpression(e));

    auto result = hilti::builder::tuple::create(elems, t->location());
    setResult(result);
}

void CodeBuilder::visit(ctor::Bytes* b)
{
    auto result = hilti::builder::bytes::create(b->value(), b->location());
    setResult(result);
}

void CodeBuilder::visit(ctor::List* l)
{
    auto ltype = ast::checkedCast<type::List>(l->type());
    auto etype = cg()->hiltiType(ltype->elementType());

    hilti::builder::list::element_list elems;

    for ( auto e : l->elements() )
        elems.push_back(cg()->hiltiExpression(e));

    auto result = hilti::builder::list::create(etype, elems, l->location());
    setResult(result);
}

void CodeBuilder::visit(ctor::Map* m)
{
}

void CodeBuilder::visit(ctor::RegExp* r)
{
}

void CodeBuilder::visit(ctor::Set* s)
{
}

void CodeBuilder::visit(ctor::Vector* v)
{
}

void CodeBuilder::visit(declaration::Constant* c)
{
}

void CodeBuilder::visit(declaration::Function* f)
{
    if ( f->linkage() == Declaration::IMPORTED )
        return;

    cg()->hiltiDefineFunction(f->function());
}

void CodeBuilder::visit(declaration::Hook* h)
{
    cg()->hiltiDefineHook(h->id(), h->hook());
}

void CodeBuilder::visit(declaration::Type* t)
{
    auto unit = ast::tryCast<type::Unit>(t->type());

    auto id = cg()->hiltiID(t->id());
    auto type = unit ? cg()->hiltiTypeParseObject(unit) : cg()->hiltiType(t->type());

    cg()->moduleBuilder()->addType(id, type, false, t->location());

    // If this is an exported unit type, generate the parsing functions for
    // it.
    if ( unit && t->linkage() == Declaration::EXPORTED )
        cg()->hiltiExportParser(unit);

    // For units, generate the embedded hooks.
    if ( unit )
        cg()->hiltiUnitHooks(unit);
}

void CodeBuilder::visit(declaration::Variable* v)
{
    auto var = v->variable();
    auto id = cg()->hiltiID(v->id());
    auto type = cg()->hiltiType(var->type());

    auto local = ast::as<variable::Local>(var);
    auto global = ast::as<variable::Global>(var);

    if ( local ) {
        auto local = cg()->moduleBuilder()->addLocal(id, type, nullptr, nullptr, v->location());
        auto hltinit = var->init() ? cg()->hiltiExpression(var->init()) : cg()->hiltiDefault(var->type(), true);

        if ( hltinit )
            cg()->builder()->addInstruction(local, hilti::instruction::operator_::Assign, hltinit);
    }

    else if ( global ) {
        auto global = cg()->moduleBuilder()->addGlobal(id, type, nullptr, nullptr, v->location());
        cg()->moduleBuilder()->pushModuleInit();

        auto hltinit = var->init() ? cg()->hiltiExpression(var->init()) : cg()->hiltiDefault(var->type(), true);

        if ( hltinit )
            cg()->builder()->addInstruction(global, hilti::instruction::operator_::Assign, hltinit);

        cg()->moduleBuilder()->popModuleInit();
    }
}

void CodeBuilder::visit(expression::Assign* a)
{
    auto target = cg()->hiltiExpression(a->destination());
    auto op1 = cg()->hiltiExpression(a->source(), a->destination()->type());
    cg()->builder()->addInstruction(target, hilti::instruction::operator_::Assign, op1);
    setResult(target);
}

void CodeBuilder::visit(expression::CodeGen* c)
{
    setResult(c->value());
}

void CodeBuilder::visit(expression::Coerced* c)
{
}

void CodeBuilder::visit(expression::Constant* c)
{
    shared_ptr<hilti::Node> result = nullptr;
    bool success = processOne(c->constant(), &result);
    assert(success);
    assert(result);
    setResult(result);
}

void CodeBuilder::visit(expression::Ctor* c)
{
    shared_ptr<hilti::Node> result = nullptr;
    bool success = processOne(c->ctor(), &result);
    assert(success);
    assert(result);
    setResult(result);
}

void CodeBuilder::visit(expression::Default* c)
{
    auto result = cg()->hiltiDefault(c->type());
    setResult(result);
}

void CodeBuilder::visit(expression::Function* f)
{
    auto id = cg()->hiltiFunctionName(f->sharedPtr<expression::Function>());
    auto result = hilti::builder::id::create(id, f->location());
    setResult(result);
}

void CodeBuilder::visit(expression::ID* i)
{
}

void CodeBuilder::visit(expression::List* l)
{
}

void CodeBuilder::visit(expression::MemberAttribute* m)
{
}

void CodeBuilder::visit(expression::Module* m)
{
}

void CodeBuilder::visit(expression::Parameter* p)
{
    auto result = hilti::builder::id::create(cg()->hiltiID(p->parameter()->id()), p->location());
    setResult(result);
}

void CodeBuilder::visit(expression::ParserState* p)
{
    shared_ptr<hilti::Expression> expr = nullptr;

    switch ( p->kind() ) {
     case expression::ParserState::SELF:
        expr = hilti::builder::id::create("__self");
        break;

     case expression::ParserState::DOLLARDOLLAR:
        if ( ! _dollardollar )
            internalError("$$ not bound in CodeBuilder::visit(expression::ParserState* p)");

        expr = _dollardollar;
        break;

     case expression::ParserState::PARAMETER: {
         auto fname = hilti::builder::string::create(util::fmt("__p_%s", p->id()->name()));
         auto ftype = cg()->hiltiType(p->type());
         expr = cg()->moduleBuilder()->addTmp("param", ftype, nullptr, false);
         cg()->builder()->addInstruction(expr, hilti::instruction::struct_::Get, cg()->hiltiSelf(), fname);
         break;
     }

     default:
        internalError("unknown ParserState kind");
    }

    setResult(expr);
}

void CodeBuilder::visit(expression::Type* t)
{
}

void CodeBuilder::visit(expression::UnresolvedOperator* u)
{
}

void CodeBuilder::visit(expression::Variable* v)
{
    call(v->variable());
}

void CodeBuilder::visit(statement::Block* b)
{
    for ( auto d : b->declarations() )
        processOne(d);

    for ( auto s : b->statements() )
        hiltiStatement(s);
}

void CodeBuilder::visit(statement::Expression* e)
{
    cg()->hiltiExpression(e->expression());
}

void CodeBuilder::visit(statement::ForEach* f)
{
}

void CodeBuilder::visit(statement::IfElse* i)
{
    auto cond = cg()->hiltiExpression(i->condition());
    auto cont = cg()->moduleBuilder()->newBuilder("if-cont");
    auto true_ = cg()->moduleBuilder()->newBuilder("if-true");
    auto false_ = i->statementFalse() ? cg()->moduleBuilder()->newBuilder("if-false")
                                      : shared_ptr<hilti::builder::BlockBuilder>();

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, true_->block(),
                                    false_ ? false_->block() : cont->block());

    cg()->moduleBuilder()->pushBuilder(true_);
    cg()->hiltiStatement(i->statementTrue());

    if ( ! cg()->builder()->statement()->terminated() )
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());

    cg()->moduleBuilder()->popBuilder(true_);

    if ( i->statementFalse() ) {
        cg()->moduleBuilder()->pushBuilder(false_);
        cg()->hiltiStatement(i->statementFalse());

        if ( ! cg()->builder()->statement()->terminated() )
            cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());

        cg()->moduleBuilder()->popBuilder(false_);
    }

    cg()->moduleBuilder()->pushBuilder(cont);

    // Leave on stack.
}

void CodeBuilder::visit(statement::NoOp* n)
{
}

void CodeBuilder::visit(statement::Print* p)
{
    auto loc = p->location();
    auto print = hilti::builder::id::create("Hilti::print", loc);

    auto exprs = p->expressions();

    if ( ! exprs.size() ) {
        // Just print an empty line.
        auto t = hilti::builder::tuple::create({ hilti::builder::string::create("", loc) }, loc);
        builder()->addInstruction(hilti::instruction::flow::CallVoid, print, t);
        return;
    }

    auto e = exprs.begin();

    while ( e != exprs.end() ) {
        if ( e != exprs.begin() ) {
            auto op = hilti::builder::string::create(" ");
            auto t = hilti::builder::tuple::create({ op, hilti::builder::boolean::create(false, loc) }, loc);
            builder()->addInstruction(hilti::instruction::flow::CallVoid, print, t);
        }

        auto op = hiltiExpression(*e++);

        if ( e != exprs.end() ) {
            auto t = hilti::builder::tuple::create({ op, hilti::builder::boolean::create(false, loc) }, loc);
            builder()->addInstruction(hilti::instruction::flow::CallVoid, print, t);
        }

        else {
            auto t = hilti::builder::tuple::create({ op }, loc);
            builder()->addInstruction(hilti::instruction::flow::CallVoid, print, t);
        }
    }
}

void CodeBuilder::visit(statement::Return* r)
{
    if ( ! r->expression() )
        builder()->addInstruction(hilti::instruction::flow::ReturnVoid);

    else {
        auto val = cg()->hiltiExpression(r->expression());
        builder()->addInstruction(hilti::instruction::flow::ReturnResult, val);
    }
}

void CodeBuilder::visit(statement::Stop* r)
{
    builder()->addInstruction(hilti::instruction::hook::Stop, hilti::builder::boolean::create(true));
}

void CodeBuilder::visit(statement::Try* t)
{
}

void CodeBuilder::visit(statement::try_::Catch* c)
{
}

///////// Operators

static shared_ptr<hilti::type::Integer> _intResultType(binpac::expression::ResolvedOperator* op)
{
    auto t1 = ast::checkedCast<binpac::type::Integer>(op->op1()->type());
    auto t2 = ast::checkedCast<binpac::type::Integer>(op->op2()->type());

    int w = std::max(t1->width(), t2->width());
    return hilti::builder::integer::type(w);
}

void CodeBuilder::visit(expression::operator_::integer::Minus* i)
{
    auto result = builder()->addTmp("sum", _intResultType(i));
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Sub, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Plus* i)
{
    auto result = builder()->addTmp("diff", _intResultType(i));
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Add, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::integer::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::integer::Attribute* i)
{
    auto itype = ast::checkedCast<type::Integer>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto bits = itype->bits(attr->id());
    assert(bits);

    auto result = cg()->builder()->addTmp("bits", cg()->hiltiType(itype));

    cg()->builder()->addInstruction(result,
                                    hilti::instruction::integer::Mask,
                                    cg()->hiltiExpression(i->op1()),
                                    hilti::builder::integer::create(bits->lower()),
                                    hilti::builder::integer::create(bits->upper()));

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::bool_::Not* i)
{
    auto result = builder()->addTmp("not", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    cg()->builder()->addInstruction(result, hilti::instruction::boolean::Not, op1);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::unit::Attribute* i)
{
    auto unit = ast::checkedCast<type::Unit>(i->op1()->type());
    auto attr = ast::checkedCast<expression::MemberAttribute>(i->op2());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto ival = cg()->builder()->addTmp("item", cg()->hiltiType(cg()->itemType(item)), nullptr, false);
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
    auto expr = cg()->hiltiExpression(i->op3());

    auto item = unit->item(attr->id());
    assert(item && item->type());

    auto ival = cg()->builder()->addTmp("item", cg()->hiltiType(cg()->itemType(item)), nullptr, false);
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    cg()->hiltiExpression(i->op1()),
                                    hilti::builder::string::create(attr->id()->name()),
                                    expr);

    cg()->hiltiRunFieldHooks(item);

    setResult(expr);
}

void binpac::codegen::CodeBuilder::visit(binpac::expression::operator_::unit::Input* i)
{
}

void binpac::codegen::CodeBuilder::visit(binpac::expression::operator_::unit::HasAttribute* i)
{
}

void binpac::codegen::CodeBuilder::visit(binpac::expression::operator_::unit::New* i)
{
}

void binpac::codegen::CodeBuilder::visit(binpac::expression::operator_::unit::Offset* i)
{
}

void binpac::codegen::CodeBuilder::visit(binpac::expression::operator_::unit::SetInput* i)
{
}

void binpac::codegen::CodeBuilder::visit(binpac::expression::operator_::unit::AddFilter* i)
{
}

void CodeBuilder::visit(expression::operator_::list::PushBack* i)
{
    auto op1 = cg()->hiltiExpression(i->op1());

    auto tuple = ast::checkedCast<expression::Constant>(i->op3())->constant();
    auto params = ast::checkedCast<constant::Tuple>(tuple)->value();

    auto param1 = cg()->hiltiExpression(*params.begin());
    cg()->builder()->addInstruction(hilti::instruction::list::PushBack, op1, param1);

    setResult(op1);
}

void CodeBuilder::visit(expression::operator_::function::Call* i)
{
    auto func = ast::checkedCast<expression::Function>(i->op1());
    auto args = ast::checkedCast<expression::List>(i->op2())->expressions();

    shared_ptr<hilti::Expression> cookie = nullptr;

    if ( in<declaration::Function>() )
        cookie = hilti::builder::id::create("__cookie", i->location());
    else
        cookie = hilti::builder::reference::createNull(i->location());

    auto result = cg()->hiltiCall(func, args, cookie);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::enum_::Call* i)
{
    auto type = ast::checkedCast<type::TypeType>(i->op1()->type())->typeType();
    auto etype = ast::checkedCast<type::Enum>(type);
    auto args = ast::checkedCast<expression::List>(i->op2())->expressions();
    assert(args.size() == 1);
    auto value = args.front();

    auto result = cg()->builder()->addTmp("enum", cg()->hiltiType(etype));
    cg()->builder()->addInstruction(result, hilti::instruction::enum_::FromInt, cg()->hiltiExpression(value));

    setResult(result);
}

void CodeBuilder::visit(expression::operator_::tuple::Equal* i)
{
    auto result = builder()->addTmp("equal", hilti::builder::boolean::type());
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::tuple::Equal, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(expression::operator_::tuple::Index* i)
{
    auto tuple = ast::checkedCast<type::Tuple>(i->op1()->type());
    auto types = tuple->typeList();

    auto idx = ast::checkedCast<expression::Constant>(i->op2());
    auto const_ = ast::checkedCast<constant::Integer>(idx->constant());

    auto t = types.begin();
    std::advance(t, const_->value());

    auto result = builder()->addTmp("elem", cg()->hiltiType(*t));
    auto op1 = cg()->hiltiExpression(i->op1());
    auto op2 = cg()->hiltiExpression(i->op2());
    cg()->builder()->addInstruction(result, hilti::instruction::tuple::Index, op1, op2);
    setResult(result);
}

void CodeBuilder::visit(variable::Local* v)
{
    auto result = hilti::builder::id::create(cg()->hiltiID(v->id()));
    setResult(result);
}

void CodeBuilder::visit(variable::Global* v)
{
    auto result = hilti::builder::id::create(cg()->hiltiID(v->id()));
    setResult(result);
}
