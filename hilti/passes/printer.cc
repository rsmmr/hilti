
#include <arpa/inet.h>

#include "hilti-intern.h"

using namespace hilti;
using namespace hilti::passes;
using namespace ast::passes::printer; // For 'endl'.

Printer::Printer(std::ostream& out, bool single_line)
    : ast::passes::Printer<AstInfo>(out, single_line)
{
}

void Printer::visit(Module* m)
{
    Printer& p = *this;

    p << "module " << m->id() << endl;
    p << endl;

    bool sep = false;

    for ( auto i : m->importedIDs() ) {
        if ( i->name() == "libhilti" )
            // Will always be implicitly loaded.
            continue;

        p << "import " << i << endl;
        sep = true;
    }

    if ( sep )
        p << endl;

    sep = false;

    for ( auto i : m->exportedIDs(false) ) {
        p << "export " << i << endl;
        sep = true;
    }

    if ( sep )
        p << endl;

    sep = false;

    for ( auto i : m->exportedTypes() ) {
        p << "export " << i << endl;
        sep = true;
    }

    if ( sep )
        p << endl;

    p << m->body();
}

void Printer::visit(statement::Block* b)
{
    Printer& p = *this;

//    p << "==== Block start" << endl;

    bool in_function = in<declaration::Function>();

    if ( in_function ) {
        if ( b->id() && b->id()->pathAsString().size() )
            p << no_indent << b->id() << ":" << endl;

        if ( parent<statement::Block>() )
            pushIndent();
    }

    for ( auto d : b->declarations() )
        p << d << endl;

    if ( b->declarations().size() )
        p << endl;

    for ( auto s : b->statements() )
        p << s << endl;

    if ( in_function && parent<statement::Block>() )
        popIndent();

//    p << "==== Block end" << endl;
}

void Printer::visit(statement::Try* s)
{
    Printer& p = *this;

    p << "try {" << endl;
    p << s->block();
    p << "}" << endl;
    p << endl;

    for ( auto c : s->catches() )
        p << c;

    p << endl;
}

void Printer::visit(statement::try_::Catch* c)
{
    Printer& p = *this;

    if ( c->catchAll() )
        p << "catch {" << endl;

    else {
        p << "catch ( ";
        p << c->type();
        p << ' ';
        p << c->id();
        p << " ) {" << endl;
    }

    p << c->block();
    p << "}" << endl;
    p << endl;
}

void Printer::visit(statement::ForEach* s)
{
    Printer& p = *this;

    p << "for ( " << s->id()->name() << " in " << s->sequence() << " ) {" << endl;
    p << s->body();
    p << "}" << endl;
    p << endl;
}

static void printInstruction(Printer& p, statement::Instruction* i)
{
    if ( i->internal() )
        return;

    if ( i->comment().size() )
        p << "# " << i->comment() << endl;

    auto ops = i->operands();

    if ( ops[0] )
        p << ops[0] << " = ";

    string id = util::strreplace(i->id()->name(), ".op.", "");

    p << id;

    for ( int i = 1; i < ops.size(); ++i ) {
        if ( ops[i] )
            p << " " << ops[i];
    }
}

void Printer::visit(statement::instruction::Resolved* i)
{
    printInstruction(*this, i);
}

void Printer::visit(statement::instruction::Unresolved* i)
{
    printInstruction(*this, i);
}

void Printer::visit(expression::Constant* e)
{
    Printer& p = *this;
    p << e->constant();
}

void Printer::visit(expression::Coerced* e)
{
    Printer& p = *this;
    p << e->expression();
}

void Printer::visit(expression::Ctor* e)
{
    Printer& p = *this;
    p << e->ctor();
}

void Printer::visit(expression::ID* i)
{
    Printer& p = *this;
    p << scopedID(i, i->id());
}

void Printer::visit(expression::Variable* v)
{
    Printer& p = *this;
    p << scopedID(v, v->variable()->id());
}

void Printer::visit(expression::Parameter* pa)
{
    Printer& p = *this;
    p << pa->parameter()->id();
}

void Printer::visit(expression::Function* f)
{
    Printer& p = *this;
    p << scopedID(f, f->function()->id());
}

void Printer::visit(expression::Module* m)
{
    Printer& p = *this;
    p << scopedID(m, m->module()->id());
}

void Printer::visit(expression::Type* t)
{
    Printer& p = *this;
    auto id = t->type()->id();
    p << (id ? scopedID(t, id) : t->type()->render());
}

void Printer::visit(expression::Block* b)
{
    Printer& p = *this;
    if ( b->block()->id() )
        p << b->block()->id();
}

void Printer::visit(expression::CodeGen* c)
{
    Printer& p = *this;
    p << "<internal code generator expression>";
}

void Printer::visit(declaration::Variable* v)
{
    Printer& p = *this;

    const char* tag = ast::as<variable::Local>(v->variable()) ? "local" : "global";

    p << tag << " " << v->variable()->type() << " " << v->id();

    if ( v->variable()->init() ) {
        p << " = ";
        p << v->variable()->init();
    }
}

void Printer::visit(declaration::Type* t)
{
    Printer& p = *this;

    disableTypeIDs();

    auto is_ctx = ast::isA<type::Context>(t->type());

    if ( ! is_ctx )
        p << "type " << t->id() << " = ";

    p << t->type();

    enableTypeIDs();
}

void Printer::visit(declaration::Function* f)
{
    Printer& p = *this;

    auto func = f->function();
    auto ftype = func->type();

    shared_ptr<Hook> hook = nullptr;

    auto hook_decl = dynamic_cast<declaration::Hook *>(f);
    if ( hook_decl )
        hook = hook_decl->hook();

    bool has_impl = static_cast<bool>(func->body());

    if ( func->initFunction() )
        p << "init ";

    if ( ! has_impl )
        p << "declare ";

    if ( hook )
        p << "hook ";

    switch ( ftype->callingConvention() ) {
     case type::function::HILTI_C:
        p << "\"C-HILTI\" ";
        break;

     case type::function::C:
        p << "\"C\" ";
        break;

     case type::function::HILTI:
     case type::function::HOOK:
        // Default.
        break;

     default:
        internalError("unknown calling convention");
    }

    p << ftype->result() << " " << f->id() << '(';

    printList(ftype->parameters(), ", ");

    p << ')';

    if ( hook ) {
        if ( hook->priority() != 0 )
            p << " &priority=" << hook->priority() << " ";

        if ( hook->group() != 0 )
            p << " &group=" << hook->group() << " ";
    }

    p << endl;

    if ( has_impl ) {
        p << '{' << endl;
        p << func->body();
        p << '}' << endl;
        p << endl;
    }
}

void Printer::visit(type::function::Parameter* param)
{
    Printer& p = *this;

    if ( param->constant() && ! ast::isA<type::Void>(param->type()) )
        p << "const ";

    if ( param->type() )
        p << param->type();

    if ( param->id() )
        p << ' ' << param->id();

    auto def = param->default_();

    if ( def )
        p << '=' << def;
}

void Printer::visit(type::function::Result* r)
{
    Printer& p = *this;
    p << r->type();
}

void Printer::visit(type::Function * t)
{
    Printer& p = *this;
    p << "<function>";
}

void Printer::visit(type::Hook* t)
{
    Printer& p = *this;
    p << "<hook>";
}


void Printer::visit(ID* i)
{
    Printer& p = *this;
    p << i->pathAsString();
}

void Printer::visit(type::Any* i)
{
    Printer& p = *this;
    p << "any";
}

void Printer::visit(type::Void* i)
{
    Printer& p = *this;
    p << "void";
}

void Printer::visit(type::Unknown* i)
{
    Printer& p = *this;
    p << (i->id() ? util::fmt("<Unresolved '%s'>", i->id()->pathAsString().c_str()) : "<Unknown>");
}

void Printer::visit(type::Integer* i)
{
    if ( printTypeID(this, i) )
        return;

    Printer& p = *this;

    if ( i->wildcard() )
        p << "int<*>";
    else
        p << "int<" << i->width() << ">";
}

void Printer::visit(type::String* i)
{
    if ( printTypeID(this, i) )
        return;

    Printer& p = *this;
    p << "string";
}

void Printer::visit(type::Label* i)
{
    if ( printTypeID(this, i) )
        return;

    Printer& p = *this;
    p << "label";
}

void Printer::visit(type::Bool* b)
{
    if ( printTypeID(this, b) )
        return;

    Printer& p = *this;
    p << "bool";
}

void Printer::visit(type::Reference* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    enableTypeIDs();

    if ( t->argType() )
        p << "ref<" << t->argType() << ">";
    else
        p << "ref<*>";

    disableTypeIDs();
}

void Printer::visit(type::Exception* e)
{
    if ( printTypeID(this, e) )
        return;

    Printer& p = *this;

    enableTypeIDs();

    if ( e->argType() )
        p << "exception<" << e->argType() << ">";
    else
        p << "exception";

    if ( e->baseType() )
        p << " : " << e->baseType();

    disableTypeIDs();
}

void Printer::visit(type::Bytes* b)
{
    if ( printTypeID(this, b) )
        return;

    Printer& p = *this;
    p << "bytes";
}

void Printer::visit(type::Tuple* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    p << "tuple<";
    printList(t->typeList(), ", ");
    p << ">";
}

void Printer::visit(type::TypeType* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    if ( t->typeType() )
        p << t->typeType();
    else
        p << "(no type)";
}

void Printer::visit(type::Address* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "addr";
}

void Printer::visit(type::Bitset* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;

    p << "bitset { ";

    bool first = true;

    for ( auto l : c->labels() ) {

        if ( ! first )
            p << ", ";

        p << l.first->pathAsString();

        if ( l.second >= 0 )
                p << " = " << l.second;

        first = false;
    }
}

void Printer::visit(type::Scope* s)
{
    if ( printTypeID(this, s) )
        return;

    Printer& p = *this;

    p << "scope { ";

    bool first = true;

    for ( auto l : s->fields() ) {

        if ( ! first )
            p << ", ";

        p << l;

        first = false;
    }

    p << "} " << endl;
}

void Printer::visit(type::CAddr* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "caddr";
}

void Printer::visit(type::Double* c)
{
    if ( printTypeID(this, c) )
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

    bool first = true;

    for ( auto l : c->labels() ) {

        if ( *l.first == "Undef" )
            continue;

        if ( ! first )
            p << ", ";

        p << l.first->pathAsString();

        if ( l.second >= 0 )
                p << " = " << l.second;

        first = false;
    }
}

void Printer::visit(type::Interval* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "interval";
}

void Printer::visit(type::Time* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "time";
}

void Printer::visit(type::Network* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "net";
}

void Printer::visit(type::Port* c)
{
    if ( printTypeID(this, c) )
        return;

    Printer& p = *this;
    p << "port";
}

void Printer::visit(type::Callable* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "callable";
}

void Printer::visit(type::Channel* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "channel<" << t->argType() << ">";
    else
        p << "channel<*>";
}

void Printer::visit(type::Classifier* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( ! t->wildcard() )
        p << "classifier<" << t->ruleType() << "," << t->valueType() << ">";
    else
        p << "classifier<*>";
}

void Printer::visit(type::File* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "file";
}

void Printer::visit(type::IOSource* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( t->kind() )
        p << "iosrc<" << t->kind() << ">";
    else
        p << "iosrc<*>";
}

void Printer::visit(type::List* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "list<" << t->argType() << ">";
    else
        p << "list<*>";
}

void Printer::visit(type::Map* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    assert(! (t->keyType() || t->valueType()) || (t->keyType() && t->valueType()));

    if ( t->keyType() )
        p << "map<" << t->keyType() << ", " << t->valueType() << ">";
    else
        p << "map<*>";
}


void Printer::visit(type::Vector* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "vector<" << t->argType() << ">";
    else
        p << "vector<*>";
}

void Printer::visit(type::Set* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "set<" << t->argType() << ">";
    else
        p << "set<*>";
}

void Printer::visit(type::Overlay* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( ! t->fields().size() ) {
        p << "overlay" << endl;
        return;
    }

    p << "overlay {" << endl;
    pushIndent();

    bool first = true;
    for ( auto f : t->fields() ) {

        if ( ! first )
            p << "," << endl;

        p << f->name() << ": ";

        if ( f->startOffset() >= 0 )
            p << "at " << f->startOffset();
        else
            p << "after " << f->startField()->name();

        p << " unpack with " << f->format();

        if ( f->formatArg() )
            p << " " << f->formatArg();

        first = false;
    }

    p << endl;
    popIndent();
    p << "}" << endl << endl;
}

void Printer::visit(type::OptionalArgument* t)
{
    Printer& p = *this;

    if ( t->argType() )
        p << t->argType();
}

void Printer::visit(type::RegExp* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "regexp<" << util::strjoin(t->attributes(), ",") << ">";
}

void Printer::visit(type::MatchTokenState* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "match_token_state";
}

void Printer::visit(type::Struct* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    auto kind = ast::isA<type::Context>(t->sharedPtr<type::Struct>()) ? "context" : "struct";

    if ( ! t->fields().size() ) {
        p << kind << endl;
        return;
    }

    p << kind << " {" << endl;
    pushIndent();
    enableTypeIDs();

    bool first = true;
    for ( auto f : t->fields() ) {

        if ( f->internal() )
            continue;

        if ( ! first )
            p << "," << endl;

        p << f->type() << ' ' << f->id();
        if ( f->default_() )
            p << " &default=" << f->default_();

        first = false;
    }

    p << endl;

    disableTypeIDs();
    popIndent();

    p << "}" << endl << endl;
}

void Printer::visit(type::Timer* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "timer";
}

void Printer::visit(type::TimerMgr* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "timer_mgr";
}

void Printer::visit(type::Iterator* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "iterator<" << t->argType() << ">";
    else
        p << "iterator<*>";
}

void Printer::visit(type::Unset* t)
{
    if ( printTypeID(this, t) )
        return;

    Printer& p = *this;
    p << "<type \"unset\">";
}

void Printer::visit(constant::Integer* i)
{
    Printer& p = *this;
    p << i->value();
}

void Printer::visit(constant::String* s)
{
    Printer& p = *this;
    p << '"' << ::util::escapeUTF8(s->value()) << '"';
}

void Printer::visit(constant::Bool* b)
{
    Printer& p = *this;
    p << (b->value() ? "True" : "False");
}

void Printer::visit(constant::Label* b)
{
    Printer& p = *this;
    p << b->value();
}

void Printer::visit(constant::Reference* r)
{
    Printer& p = *this;
    p << "Null"; // Only possible constant.
}

void Printer::visit(constant::Tuple* t)
{
    Printer& p = *this;

    p << '(';
    printList(t->value(), ", ");
    p << ')';
}

void Printer::visit(constant::Unset* t)
{
    Printer& p = *this;

    if ( ! in<constant::Tuple>() )
        internalError("printing supports unset value only in tuples");

    p << '*';
}

static void _printAddr(Printer& p, const constant::AddressVal& addr)
{
    char buffer[INET6_ADDRSTRLEN];
    const char* result;

    switch ( addr.family ) {

     case constant::AddressVal::IPv4:
        result = inet_ntop(AF_INET, &addr.in.in4, buffer, INET6_ADDRSTRLEN);
        break;

     case constant::AddressVal::IPv6:
        result = inet_ntop(AF_INET6, &addr.in.in6, buffer, INET6_ADDRSTRLEN);
        break;

     default:
        assert(false);
    }

    if ( result )
        p << result;
    else
        p << "<bad IP address>";
}

void Printer::visit(constant::Address* c)
{
    Printer& p = *this;
    _printAddr(p, c->value());
}

void Printer::visit(constant::Network* c)
{
    Printer& p = *this;
    _printAddr(p, c->prefix());
    p << '/' << c->width();
}

void Printer::visit(constant::Bitset* c)
{
    Printer& p = *this;
    auto expr = c->firstParent<Expression>();

    std::list<string> bits;

    for ( auto b : c->value() )
        bits.push_back(scopedID(expr.get(), b));

    printList(bits, " | ");
}

void Printer::visit(constant::Double* c)
{
    Printer& p = *this;
    p << c->value();

}

void Printer::visit(constant::Enum* c)
{
    Printer& p = *this;

    auto expr = c->firstParent<Expression>();
    p << scopedID(expr.get(), c->value());
}

void Printer::visit(constant::Interval* c)
{
    Printer& p = *this;

    p << "interval(" << std::showpoint << (c->value() / 1e9) << ")";
}

void Printer::visit(constant::Time* c)
{
    Printer& p = *this;

    p << "time(" << std::showpoint << (c->value() / 1e9) << ")";
}

void Printer::visit(constant::Port* c)
{
    Printer& p = *this;

    auto v = c->value();

    switch ( v.proto ) {
     case constant::PortVal::TCP:
        p << v.port << "/tcp";
        break;

     case constant::PortVal::UDP:
        p << v.port << "/udp";
        break;

     default:
        internalError("unknown protocol");
    }
}

void Printer::visit(ctor::Bytes* c)
{
    Printer& p = *this;
    p << 'b' << '"' << ::util::escapeBytes(c->value()) << '"';
}

void Printer::visit(ctor::List* c)
{
    auto rtype = ast::checkedCast<type::Reference>(c->type());
    auto etype = ast::checkedCast<type::List>(rtype->argType())->elementType();

    Printer& p = *this;

    p << "list<" << etype << ">(";
    printList(c->elements(), ", ");
    p << ")";
}

void Printer::visit(ctor::Map* c)
{
    Printer& p = *this;

    bool first = true;
    for ( auto e: c->elements() ) {
        if ( ! first )
            p << ", ";

        p << e.first << ": " << e.second;

        first = false;
    }
}

void Printer::visit(ctor::Set* c)
{
    auto rtype = ast::checkedCast<type::Reference>(c->type());
    auto etype = ast::checkedCast<type::Set>(rtype->argType())->elementType();

    Printer& p = *this;

    p << "set<" << etype << ">(";
    printList(c->elements(), ", ");
    p << ")";
}


void Printer::visit(ctor::Vector* c)
{
    auto rtype = ast::checkedCast<type::Reference>(c->type());
    auto etype = ast::checkedCast<type::Vector>(rtype->argType())->elementType();

    Printer& p = *this;

    p << "vector<" << etype << ">(";
    printList(c->elements(), ", ");
    p << ")";
}

void Printer::visit(ctor::RegExp* c)
{
    Printer& p = *this;

    std::list<string> patterns;

    for ( auto p : c->patterns() )
        patterns.push_back(string("/") + p.first + string("/") + p.second);

    printList(patterns, " | ");
}

