
#include <arpa/inet.h>

#include "printer.h"

#include "attribute.h"
#include "constant.h"
#include "ctor.h"
#include "declaration.h"
#include "expression.h"
#include "module.h"
#include "statement.h"
#include "type.h"
#include "variable.h"

using namespace binpac;
using namespace binpac::passes;
using namespace ast::passes::printer; // For 'endl'.

static void _printUnitHooks(Printer& p, const hook_list& hooks)
{
    if ( hooks.size() ) {
        for ( auto h : hooks )
            p << h;
    }
    else
        p << ";";
}

static void _printUnitFieldCommon(Printer& p, type::unit::item::Field* i)
{
    if ( i->condition() )
        p << "if ( " << i->condition() << " ) ";

    if ( i->sinks().size() ) {
        p << "-> ";
        p.printList(i->sinks(), ", ");
    }

    _printUnitHooks(p, i->hooks());

    p << endl;
}

static string _linkage(Declaration::Linkage linkage)
{
    switch( linkage ) {
     case Declaration::PRIVATE:
        return "";

     case Declaration::EXPORTED:
        return "export ";

     default:
        fprintf(stderr, "unknown linkage %d in Printer::_linkage()", (int)linkage);
        abort();
    }

    // Can't be reached.
    return "";
}

Printer::Printer(std::ostream& out, bool single_line)
    : ast::passes::Printer<AstInfo>(out, single_line)
{
}

void Printer::visit(Attribute* a)
{
    Printer& p = *this;

    p << '&' << a->key();

    if ( a->value() )
        p << '(' << a->value() << ')';
}

void Printer::visit(AttributeSet* a)
{
    Printer& p = *this;

    printList(a->attributes(), " ");
}

void Printer::visit(Function* f)
{
    Printer& p = *this;

    auto r = f->type()->result();

    if ( r->constant() )
        p << "const ";

    p << r->type() << ' ' << f->id() << '(';

    bool first = true;
    for ( auto param : f->type()->parameters() ) {

        if ( ! first )
            p << ", ";

        if ( param->constant() )
            p << "const ";

        p << param->id() << ": " << param->type();

        if ( param->default_() )
            p << " = " << param->default_();

        first = false;
    }

    p << ')';

    if ( f->body() ) {
        pushIndent();
        p << f->body();
        popIndent();
    }

    else
        p << ';';
}

void Printer::visit(binpac::Hook* h)
{
    Printer& p = *this;

    if ( h->debug() )
        p << "debug ";

    if ( h->foreach() )
        p << "foreach ";

    pushIndent();
    p << h->body();
    popIndent();
}

void Printer::visit(ID* i)
{
    Printer& p = *this;
    p << i->pathAsString();
}

void Printer::visit(Module* m)
{
    Printer& p = *this;

    p << "module " << m->id() << ";" << endl;
    p << endl;

    bool sep = false;

    for ( auto i : m->importedIDs() ) {
        p << "import " << i << endl;
        sep = true;
    }

    p << m->body();
}

void Printer::visit(constant::Address* a)
{
    Printer& p = *this;
    p << a->value();
}

void Printer::visit(constant::Bitset* b)
{
    Printer& p = *this;
    auto expr = b->firstParent<Expression>(); // may fail

    printList<shared_ptr<ID>>(b->value(), " | ", "", "",
              [&] (shared_ptr<ID> id) -> string { return scopedID(expr, id); });
}

void Printer::visit(constant::Bool* b)
{
    Printer& p = *this;
    p << (b->value() ? "True" : "False");
}

void Printer::visit(constant::Double* d)
{
    Printer& p = *this;
    p << d->value();
}

void Printer::visit(constant::Enum* e)
{
    Printer& p = *this;
    auto expr = e->firstParent<Expression>(); // may fail

    p << scopedID(expr, e->value());
}

#if 0
void Printer::visit(constant::Expression* e)
{
    Printer& p = *this;
    p << e->expression();
}
#endif

void Printer::visit(constant::Integer* i)
{
    Printer& p = *this;
    p << i->value();
}

void Printer::visit(constant::Interval* i)
{
    Printer& p = *this;
    p << "interval(" << std::showpoint << (i->value() / 1e9) << ")";
}

void Printer::visit(constant::Network* n)
{
    Printer& p = *this;
    p << n->value();
}

void Printer::visit(constant::Port* po)
{
    Printer& p = *this;
    p << po->value();
}

void Printer::visit(constant::String* s)
{
    Printer& p = *this;
    p << "\"" << util::escapeUTF8(s->value()) << "\"";
}

void Printer::visit(constant::Time* t)
{
    Printer& p = *this;
    p << "time(" << std::showpoint << (t->value() / 1e9) << ")";
}

void Printer::visit(constant::Tuple* t)
{
    Printer& p = *this;
    printList(t->value(), ", ", "(", ")");
}

void Printer::visit(ctor::Bytes* b)
{
    Printer& p = *this;
    p << "b\"" << util::escapeUTF8(b->value()) << "\"";
}

void Printer::visit(ctor::List* l)
{
    Printer& p = *this;
    printList(l->elements(), ", ", "[", "]");
}

void Printer::visit(ctor::Map* m)
{
    internalError("Cannot print maps yet");
}

void Printer::visit(ctor::RegExp* r)
{
    Printer& p = *this;

    bool first = true;

    for( auto pat : r->patterns() ) {
        if ( ! first )
            p << " | ";

        p << "/" << pat.first << "/";
        first = false;
    }
}

void Printer::visit(ctor::Set* s)
{
    printList(s->elements(), ", ", "set(", ")");
}

void Printer::visit(ctor::Vector* v)
{
    printList(v->elements(), ", ", "vector(", ")");
}

void Printer::visit(declaration::Constant* c)
{
    Printer& p = *this;
    p << _linkage(c->linkage()) << "const " << c->id() << " = " << c->constant() << ";" << endl;
}

void Printer::visit(declaration::Function* f)
{
    Printer& p = *this;
    p << _linkage(f->linkage()) << f << endl;
}

void Printer::visit(declaration::Hook* h)
{
    Printer& p = *this;

    if ( h->id() )
        p << "on " << h->id() << " ";

    p << h->hook();
}

void Printer::visit(declaration::Type* t)
{
    Printer& p = *this;

    disableTypeIDs();
    p << _linkage(t->linkage()) << "type " << t->id() << " = " << t->type() << ";" << endl;
    enableTypeIDs();
}

void Printer::visit(declaration::Variable* v)
{
    Printer& p = *this;
    p << _linkage(v->linkage()) << v->variable();
}

void Printer::visit(expression::CodeGen* c)
{
    Printer& p = *this;
    p << "(codegen expression)";
}

void Printer::visit(expression::Coerced* c)
{
    Printer& p = *this;
    p << "cast<" << c->type() << ">(" << c->expression() << ")";
}

void Printer::visit(expression::Constant* c)
{
    Printer& p = *this;
    p << c->constant();
}

void Printer::visit(expression::Ctor* c)
{
    Printer& p = *this;
    p << c->ctor();
}

void Printer::visit(expression::Function* f)
{
    Printer& p = *this;
    p << f->function()->id();
}

void Printer::visit(expression::ID* i)
{
    Printer& p = *this;
    p << i->id();
}

void Printer::visit(expression::List* l)
{
    Printer& p = *this;
    printList(l->expressions(), ", ");
}

void Printer::visit(expression::MemberAttribute* m)
{
    Printer& p = *this;
    p << m->id();
}

void Printer::visit(expression::Module* m)
{
    Printer& p = *this;
    p << m->module()->id();
}

void Printer::visit(expression::Parameter* pa)
{
    Printer& p = *this;
    p << pa->parameter()->id();
}

void Printer::visit(expression::ParserState* s)
{
    Printer& p = *this;

    switch ( s->kind() ) {
     case expression::ParserState::SELF:
        p << "self";
        break;

     case expression::ParserState::DOLLARDOLLAR:
        p << "$$";
        break;

     case expression::ParserState::PARAMETER:
        p << s->id();
        break;

     default:
        internalError("unknown ParserState kind");
    }
}

void Printer::visit(expression::ResolvedOperator* r)
{
    Printer& p = *this;

    auto exprs = r->operands();
    auto i = exprs.begin();

    auto kind = r->kind();
    auto op1 = (i != exprs.end()) ? *i++ : nullptr;
    auto op2 = (i != exprs.end()) ? *i++ : nullptr;
    auto op3 = (i != exprs.end()) ? *i++ : nullptr;

    auto o = operator_::OperatorDefinitions.find(kind);
    assert(o != operator_::OperatorDefinitions.end());

    auto op = (*o).second;

    if ( op.type == operator_::UNARY_PREFIX ) {
        p << op.display << op1;
        return;
    }

    if ( op.type == operator_::UNARY_POSTFIX ) {
        p << op1 << op.display;
        return;
    }

    if ( op.type == operator_::BINARY ) {
        p << op1 << " " << op.display << " " << op2;
        return;
    }

    switch ( op.kind ) {
     case operator_::Attribute:
        p << op1 << "." << op2;
        break;

     case operator_::AttributeAssign:
        p << op1 << "." << op2 << " = " << op3;
        break;

     case operator_::Call:
        p << op1 << "(" << op2 << ")";
        break;

     case operator_::DecrPostfix:
        p << op1 << "--";
        break;

     case operator_::HasAttribute:
        p << op1 << "?." << op2;
        break;

     case operator_::IncrPostfix:
        p << op1 << "++";
        break;

     case operator_::Index:
        p << op1 << "[" << op2 << "]";
        break;

     case operator_::IndexAssign:
        p << op1 << "[" << op2 << "] = " << op3;
        break;

     case operator_::MethodCall:
        p << op1 << "." << op2 << op3;
        break;

     case operator_::Size:
        p << "|" << op1 << "|";
        break;

     default:
        internalError(util::fmt("unknown operator %d in Printer::visit", kind));
    }
}

void Printer::visit(expression::Type* t)
{
    Printer& p = *this;
    p << t->typeValue() << endl;
}

void Printer::visit(expression::UnresolvedOperator* u)
{
    Printer& p = *this;

    p << "(Unresolved operator " << u->kind() << ": ";
    printList(u->operands(), ", ");
    p << ")";
}

void Printer::visit(expression::Variable* v)
{
    Printer& p = *this;
    p << v->variable()->id() << endl;
}

void Printer::visit(statement::Block* b)
{
    // We format the top-level block without enclosing braces.
    auto top_level = (b->parents().size() && ast::tryCast<Module>(b->parents().front()));

    Printer& p = *this;

    if ( ! top_level )
        p << "{" << endl;

    for ( auto d : b->declarations() )
        p << d;

    if ( b->declarations().size() )
        p << endl;

    for ( auto s : b->statements() )
        p << s;

    if ( ! top_level )
        p << "}" << endl;
}

void Printer::visit(statement::Expression* e)
{
    Printer& p = *this;
    p << e->expression() << endl;
}

void Printer::visit(statement::ForEach* f)
{
    Printer& p = *this;
    p << "for ( " << f->id() << " in " << f->sequence() << " )";
    p << f->body() << endl;
}

void Printer::visit(statement::IfElse* i)
{
    Printer& p = *this;
    p << "if ( " << i->condition() << " ) " << endl;
    pushIndent();
    p << i->statementTrue();
    popIndent();

    if ( i->statementFalse() ) {
        p << "else" << endl;
        pushIndent();
        p << i->statementFalse();
        popIndent();
    }

    p << endl;
}

void Printer::visit(statement::NoOp* n)
{
    // Nothing to do.
}

void Printer::visit(statement::Print* pr)
{
    Printer& p = *this;
    p << "print ";
    printList(pr->expressions(), ", ");
    p << ";" << endl;
}

void Printer::visit(statement::Return* r)
{
    Printer& p = *this;
    p << "return";

    if ( r->expression() )
        p << " " << r->expression();

    p << ";" << endl;
}

void Printer::visit(statement::Stop* r)
{
    Printer& p = *this;
    p << "stop;";
}

void Printer::visit(statement::Try* t)
{
    Printer& p = *this;
    p << "(Printer: cannot print try yet)" << endl;
}

void Printer::visit(statement::try_::Catch* c)
{
    Printer& p = *this;
    p << "(Printer: cannot print catch yet)" << endl;
}

void Printer::visit(type::Address* a)
{
    if ( printTypeID(this, a) )
        return;

    Printer& p = *this;
    p << "address";
}

void Printer::visit(type::Any* a)
{
    if ( printTypeID(this, a) )
        return;

    Printer& p = *this;
    p << "(type hook)";
}

void Printer::visit(type::Bitset* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;

    p << "bitset { ";

    bool first = false;

    for ( auto l : c->labels() ) {

        if ( ! first )
            p << ", ";

        p << l.first->pathAsString();

        if ( l.second >= 0 )
                p << " = " << l.second;

        first = false;
    }
}

void Printer::visit(type::Bool* b)
{
    if ( printTypeID(this, b) )
        return;

    Printer& p = *this;
    p << "bool";
}

void Printer::visit(type::Bytes* b)
{
    if ( printTypeID(this, b) )
        return;

    Printer& p = *this;
    p << "bytes";
}

void Printer::visit(type::CAddr* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "caddr";
}

void Printer::visit(type::Double* d)
{
    if ( printTypeID(this, d) )
        return;

    Printer& p = *this;
    p << "double";
}

void Printer::visit(type::Enum* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;

    if ( printTypeID(this, c) )
        return;

    p << "enum { ";

    bool first = false;

    for ( auto l : c->labels() ) {

        if ( ! first )
            p << ", ";

        p << l.first->pathAsString();

        if ( l.second >= 0 )
                p << " = " << l.second;

        first = false;
    }
}

void Printer::visit(type::Exception* e)
{
    if ( printTypeID(this, e) )
        return;

    Printer& p = *this;
    p << "(printer does not supoprt type exception yet)";
}

void Printer::visit(type::File* f)
{
    if ( printTypeID(this, f) )
        return;

    Printer& p = *this;
    p << "file";
}

void Printer::visit(type::Function* f)
{
    if ( printTypeID(this, f) )
        return;

    Printer& p = *this;
    p << "(type function)";
}

void Printer::visit(type::Hook* h)
{
    if ( printTypeID(this, h) )
        return;

    Printer& p = *this;
    p << "(type hook)";
}

void Printer::visit(type::Integer* i)
{
    if ( printTypeID(this, i) )
        return;

    Printer& p = *this;
    p << "int<" << i->width() << ">";
}

void Printer::visit(type::Interval* i)
{
    if ( printTypeID(this, i) )
        return;

    Printer& p = *this;
    p << "interval";
}

void Printer::visit(type::Iterator* i)
{
    if ( printTypeID(this, i) )
        return;

    Printer& p = *this;
    p << "iterator<" << i->argType() << ">";
}

void Printer::visit(type::List* l)
{
    if ( printTypeID(this, l) )
        return;

    Printer& p = *this;
    p << "list<" << l->argType() << ">";
}

void Printer::visit(type::Map* m)
{
    if ( printTypeID(this, m) )
        return;

    Printer& p = *this;
    p << "map<" << m->keyType() << "," << m->valueType() << ">";
}

void Printer::visit(type::Module* m)
{
    if ( printTypeID(this, m) )
        return;

    Printer& p = *this;
    p << "(type module)";
}

void Printer::visit(type::Network* n)
{
    if ( printTypeID(this, n) )
        return;

    Printer& p = *this;
    p << "network";
}

void Printer::visit(type::OptionalArgument* o)
{
    if ( printTypeID(this, o) )
        return;

    Printer& p = *this;
    p << "[ " << o->argType() << " ]";
}

void Printer::visit(type::Port* po)
{
    if ( printTypeID(this, po) )
        return;

    Printer& p = *this;
    p << "port";
}

void Printer::visit(type::RegExp* r)
{
    if ( printTypeID(this, r) )
        return;

    Printer& p = *this;
    p << "regexp";
}

void Printer::visit(type::Set* s)
{
    if ( printTypeID(this, s) )
        return;

    Printer& p = *this;
    p << "set<" << s->elementType() << ">";
}

void Printer::visit(type::String* s)
{
    if ( printTypeID(this, s) )
        return;

    Printer& p = *this;
    p << "string";
}

void Printer::visit(type::Time* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "time";
}

void Printer::visit(type::Tuple* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    printList(t->typeList(), ", ", "(", ")");
}

void Printer::visit(type::TypeByName* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "(type by name:" << t->id() << ")";
}

void Printer::visit(type::TypeType* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "(type type)";
}

void Printer::visit(type::Unit* u)
{
    if ( printTypeID(this, u) )
        return;

    Printer& p = *this;

    p << "unit";

    if ( u->parameters().size() )
        printList(u->parameters(), ", ", "(", ") ");

    p << " {" << endl;
    pushIndent();

    for ( auto i : u->items() )
        p << i;

    popIndent();
    p << "}";
}

void Printer::visit(type::Unknown* u)
{
    Printer& p = *this;
    p << "(unresolved type)";
}

void Printer::visit(type::Unset* u)
{
    if ( printTypeID(this, u) )
        return;

    Printer& p = *this;
    p << "(type 'unset')";
}

void Printer::visit(type::Vector* v)
{
    if ( printTypeID(this, v) )
        return;

    Printer& p = *this;
    p << "vector<" << v->elementType() << ">";
}

void Printer::visit(type::Void* v)
{
    if ( printTypeID(this, v) )
        return;

    Printer& p = *this;
    p << "void";
}

void Printer::visit(type::function::Parameter* pa)
{
    Printer& p = *this;
    p << "(type func parameter)";
}

void Printer::visit(type::function::Result* r)
{
    Printer& p = *this;
    p << "(type func result)";
}

void Printer::visit(type::unit::item::GlobalHook* g)
{
    Printer& p = *this;

    p << "on " << g->id();
    _printUnitHooks(p, g->hooks());
}

void Printer::visit(type::unit::item::Property* pr)
{
    Printer& p = *this;

    p << pr->id() << " = " << pr->value() << ";";
}

void Printer::visit(type::unit::item::Variable* v)
{
    Printer& p = *this;

    if ( v->id() )
        p << v->id();

    p << ": " << v->type();

    if ( v->default_() )
        p << " = " << v->default_();

    _printUnitHooks(p, v->hooks());
}

void Printer::visit(type::unit::item::field::Constant* c)
{
    Printer& p = *this;
    p << c->constant();

    _printUnitFieldCommon(p, c);
}

void Printer::visit(type::unit::item::field::Ctor* r)
{
    Printer& p = *this;
    if ( r->id() )
        p << r->id();

    p << ": " << r->ctor();

    _printUnitFieldCommon(p, r);
}

void Printer::visit(type::unit::item::field::Switch* s)
{
    Printer& p = *this;

    p << "switch ( " << s->expression() << " ) {" << endl;
    pushIndent();

    for ( auto c : s->cases() )
        p << c;

    popIndent();
    p << "}" << endl;
}

void Printer::visit(type::unit::item::field::AtomicType* t)
{
    Printer& p = *this;

    if ( t->id() )
        p << t->id();

    p << ": " << t->type();

    if ( t->attributes()->size() )
        p << " " << t->attributes();

    _printUnitFieldCommon(p, t);
}

void Printer::visit(type::unit::item::field::Unit* t)
{
    Printer& p = *this;

    if ( t->id() )
        p << t->id();

    p << ": " << t->type();

    if ( t->attributes()->size() )
        p << " " << t->attributes();

    if ( t->parameters().size() )
        printList(t->parameters(), ", ", "(", ") ");

    _printUnitFieldCommon(p, t);
}

void Printer::visit(type::unit::item::field::switch_::Case* c)
{
    Printer& p = *this;

    if ( c->expressions().size() )
        printList(c->expressions(), ", ");
    else
        p << "*";

    p << " -> ";
    p << c->item();
}

void Printer::visit(variable::Global* g)
{
    Printer& p = *this;
    p << "global " << g->id() << ": " << g->type();

    if ( g->init() )
        p << " = " << g->init();

    p << ";" << endl;
}

void Printer::visit(variable::Local* l)
{
    Printer& p = *this;
    p << "local " << l->id() << " : " << l->type();

    if ( l->init() )
        p << " = " << l->init();

    p << ";" << endl;
}



