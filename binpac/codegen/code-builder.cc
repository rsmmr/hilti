
#include "code-builder.h"
#include "declaration.h"
#include "expression.h"
#include "statement.h"
#include "function.h"
#include "grammar.h"

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
}

void CodeBuilder::visit(constant::Double* d)
{
}

void CodeBuilder::visit(constant::Enum* e)
{
}

void CodeBuilder::visit(constant::Expression* e)
{
}

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
}

void CodeBuilder::visit(ctor::Bytes* b)
{
}

void CodeBuilder::visit(ctor::List* l)
{
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
}

void CodeBuilder::visit(expression::CodeGen* c)
{
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
}

void CodeBuilder::visit(expression::Function* f)
{
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
}

void CodeBuilder::visit(expression::ParserState* p)
{
    shared_ptr<hilti::Expression> expr = nullptr;

    switch ( p->kind() ) {
     case expression::ParserState::SELF:
        expr = hilti::builder::id::create("__self");
        break;

     case expression::ParserState::DOLLARDOLLAR:
        expr = hilti::builder::id::create("__dollardollar");
        break;

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
}

void CodeBuilder::visit(statement::Block* b)
{
    for ( auto d : b->declarations() )
        processOne(d);

    for ( auto s : b->statements() )
        processOne(s);
}

void CodeBuilder::visit(statement::Expression* e)
{
}

void CodeBuilder::visit(statement::ForEach* f)
{
}

void CodeBuilder::visit(statement::IfElse* i)
{
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
}

void CodeBuilder::visit(statement::Try* t)
{
}

void CodeBuilder::visit(statement::try_::Catch* c)
{
}

///////// Operators

void CodeBuilder::visit(expression::operator_::integer::Minus* i)
{
}

void CodeBuilder::visit(expression::operator_::integer::Plus* i)
{
}
