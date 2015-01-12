
#include <hilti/hilti.h>

#include "code-builder.h"
#include "../declaration.h"
#include "../expression.h"
#include "../type.h"
#include "../statement.h"
#include "../function.h"
#include "../grammar.h"
#include "../constant.h"
#include "../attribute.h"

extern "C" {
#include "../../libbinpac/rtti.h"
}

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
    assert(success || errors());
}

shared_ptr<hilti::Expression> CodeBuilder::hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to)
{
    if ( coerce_to )
        expr = expr->coerceTo(coerce_to);

    shared_ptr<hilti::Node> result;
    bool success = processOne(expr, &result, coerce_to);
    assert(success || errors());

    assert(result);
    return ast::checkedCast<hilti::Expression>(result);
}

void CodeBuilder::hiltiBindDollarDollar(shared_ptr<hilti::Expression> val)
{
    _dollardollar = val;
}

expression_list CodeBuilder::callParameters(shared_ptr<Expression> tupleop)
{
    auto tuple = ast::checkedCast<expression::Constant>(tupleop)->constant();
    return ast::checkedCast<constant::Tuple>(tuple)->value();
}

shared_ptr<binpac::Expression> CodeBuilder::callParameter(shared_ptr<Expression> tupleop, int i)
{
    auto exprs = callParameters(tupleop);
    auto e = exprs.begin();
    std::advance(e, i);
    return e != exprs.end() ? *e : nullptr;
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

void CodeBuilder::visit(declaration::Constant* c)
{
}

void CodeBuilder::visit(declaration::Function* f)
{
    auto scope = current<Module>()->id()->name();
    cg()->hiltiDefineFunction(f->function(), false, scope);
}

void CodeBuilder::visit(declaration::Hook* h)
{
    cg()->hiltiDefineHook(h->id(), h->hook());
}

void CodeBuilder::visit(declaration::Type* t)
{
    auto unit = ast::tryCast<type::Unit>(t->type());
    auto type = unit ? cg()->hiltiTypeParseObject(unit) : cg()->hiltiType(t->type());

    cg()->hiltiAddType(t->id(), type, unit);

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
        auto local = cg()->moduleBuilder()->addLocal(id, type, nullptr, ::hilti::AttributeSet(), false, v->location());
        auto hltinit = var->init() ? cg()->hiltiExpression(var->init()) : cg()->hiltiDefault(var->type(), true, false);

        if ( hltinit )
            cg()->builder()->addInstruction(local, hilti::instruction::operator_::Assign, hltinit);
    }

    else if ( global ) {
        auto global = cg()->moduleBuilder()->addGlobal(id, type, nullptr, ::hilti::AttributeSet(), false, v->location());
        cg()->moduleBuilder()->pushModuleInit();

        auto hltinit = var->init() ? cg()->hiltiExpression(var->init()) : cg()->hiltiDefault(var->type(), true, false);

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
    auto expr = cg()->hiltiExpression(c->expression());
    auto coerced = cg()->hiltiCoerce(expr, c->expression()->type(), c->type());
    setResult(coerced);
}

void CodeBuilder::visit(expression::Conditional* c)
{
    auto cond = cg()->hiltiExpression(c->condition(), std::make_shared<type::Bool>());
    auto rtype = c->true_()->type();
    auto result = cg()->moduleBuilder()->addTmp("cond", cg()->hiltiType(rtype));

    auto cont = cg()->moduleBuilder()->newBuilder("cond-cont");
    auto true_ = cg()->moduleBuilder()->newBuilder("cond-true");
    auto false_ = cg()->moduleBuilder()->newBuilder("cond-false");

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, true_->block(), false_->block());

    shared_ptr<hilti::Expression> expr;

    cg()->moduleBuilder()->pushBuilder(true_);
    expr = cg()->hiltiExpression(c->true_(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign, expr);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(true_);

    cg()->moduleBuilder()->pushBuilder(false_);
    expr = cg()->hiltiExpression(c->false_(), rtype);
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Assign, expr);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(false_);


    cg()->moduleBuilder()->pushBuilder(cont);

    // Leave on stack.
    //
    setResult(result);
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
    auto result = cg()->hiltiDefault(c->type(), false, false);
    setResult(result);
}

void CodeBuilder::visit(expression::Function* f)
{
    auto id = cg()->hiltiFunctionName(f->sharedPtr<expression::Function>());
    auto result = hilti::builder::id::create(id, f->location());

    if ( f->scope() != current<Module>()->id()->name() )
        cg()->hiltiDefineFunction(f->sharedPtr<expression::Function>(), true);

    setResult(result);
}

void CodeBuilder::visit(expression::ID* i)
{
}

void CodeBuilder::visit(expression::List* l)
{
}

void CodeBuilder::visit(expression::ListComprehension* c)
{
    auto mbuilder = cg()->moduleBuilder();

    auto ltype = hilti::builder::list::type(cg()->hiltiType(c->output()->type()));
    auto output = cg()->builder()->addTmp("lcout", hilti::builder::reference::type(ltype));

    cg()->builder()->addInstruction(output, hilti::instruction::list::New, hilti::builder::type::create(ltype));

	std::shared_ptr<::hilti::ID> n(nullptr);
	mbuilder->pushBody(true);
	mbuilder->pushBuilder(n);

    auto add = mbuilder->newBuilder("add_elem");

    if ( c->predicate() ) {
        auto pred = cg()->hiltiExpression(c->predicate());
        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, pred, add->block(), ::hilti::builder::loop::next());
    }

    else
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, add->block());

    mbuilder->pushBuilder(add);
    auto val = cg()->hiltiExpression(c->output());
    cg()->builder()->addInstruction(hilti::instruction::list::PushBack, output, val);
    mbuilder->popBuilder(add);

    mbuilder->popBuilder();

	auto body = mbuilder->popBody();
	auto body_stmt = ::ast::checkedCast<hilti::statement::Block>(body->block());

    auto id = cg()->hiltiID(c->variable());
    auto input = cg()->hiltiExpression(c->input());
	auto s = ::hilti::builder::loop::foreach(id, input, body_stmt);
	cg()->builder()->addInstruction(s);

    setResult(output);
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
         auto fname = util::fmt("__p_%s", p->id()->name());
         auto ftype = cg()->hiltiType(p->type());
         expr = cg()->hiltiItemGet(cg()->hiltiSelf(), fname, ftype);
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
    auto cond = cg()->hiltiExpression(i->condition(), std::make_shared<type::Bool>());
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
    auto print = hilti::builder::id::create("BinPACHilti::print", loc);

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
        auto func = current<declaration::Function>();
        assert(func);
        auto rtype = func->function()->type()->result()->type();
        auto val = cg()->hiltiExpression(r->expression(), rtype);
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

void CodeBuilder::visit(variable::Local* v)
{
    auto result = hilti::builder::id::create(cg()->hiltiID(v->id()));
    setResult(result);
}

void CodeBuilder::visit(variable::Global* v)
{
    auto result = hilti::builder::id::create(cg()->hiltiID(v->id(), true));

    auto hid = result->id();

    if ( hid->isScoped() && ! cg()->moduleBuilder()->declared(hid) )
        moduleBuilder()->declareGlobal(hid, cg()->hiltiType(v->type()));

    setResult(result);
}

