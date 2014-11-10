
#include <arpa/inet.h>

#include "hilti/hilti-intern.h"

using namespace hilti;
using namespace hilti::passes;
using namespace ast::passes::printer; // For 'endl'.

static string _statementName(Statement* stmt)
{
    return util::fmt("[%0.4lu]", stmt->number());
}

Printer::Printer(std::ostream& out, bool single_line, bool cfg)
    : ast::passes::Printer<AstInfo>(out, single_line)
{
    _cfg = cfg;
    setPrintOriginalIDs(true);
}

bool Printer::includeFlow() const
{
    return _cfg && _module && _module->cfg();
}

static string _variableSetToString(const Statement::variable_set& vars)
{
    std::set<string> svars;

    for ( auto v : vars )
        svars.insert(v->name);

    return svars.size() ? util::strjoin(svars, ", ") : "<none>";
}

void Printer::printFlow(Statement* stmt, const string& prefix)
{
    if ( ! includeFlow() )
        return;

    auto cfg = _module->cfg();

    std::set<string> succ;
    for ( auto s : cfg->successors(stmt->sharedPtr<Statement>()) )
        succ.insert(_statementName(s.get()));

    std::set<string> pred;
    for ( auto s : cfg->predecessors(stmt->sharedPtr<Statement>()) )
        pred.insert(_statementName(s.get()));

    string spred = (pred.size() ? util::strjoin(pred, ", ") : "<none>");
    string ssucc = (succ.size() ? util::strjoin(succ, ", ") : "<none>");

    auto fi = stmt->flowInfo();
    auto sdefined = _variableSetToString(fi.defined);
    auto scleared = _variableSetToString(fi.cleared);
    auto smodified = _variableSetToString(fi.modified);
    auto sread = _variableSetToString(fi.read);

    string sin = "n/a";
    string sout = "n/a";
    string sdead = "n/a";
    string starget = "n/a";

    if ( _module->liveness() ) {
        auto liveness = _module->liveness()->liveness(stmt->sharedPtr<Statement>());
        sin = _variableSetToString(*liveness.in);
        sout = _variableSetToString(*liveness.out);
        sdead = _variableSetToString(*liveness.dead);
    }

    string s = "";

    if ( prefix.size() )
        s = prefix + " ";

    string pre = util::fmt("  \33[1;30m");
    string post = "\33[0m";

    Printer& p = *this;

    p << util::fmt("%s# %spred: { %s } succ: { %s }%s", pre, s, spred, ssucc, post) << endl;
    p << util::fmt("%s# def: { %s } clear: { %s } mod: { %s } read: { %s }", pre, sdefined, scleared, smodified, sread) << endl;
    p << util::fmt("%s# live-in: { %s } live-out: { %s }%s", pre, sin, sout, post) << endl;
    p << util::fmt("%s# now-dead: { %s }%s", pre, sdead, post) << endl;
    p << util::fmt("%s%s%s ", pre, _statementName(stmt), post);
}

bool Printer::printTypeID(Type* t)
{
    return ::ast::passes::printer::printTypeID(this, t, _module);
}

void Printer::printAttributes(const AttributeSet& attrs)
{
    for ( auto a : attrs.all() ) {
        (*this) << " &" << Attribute::tagToName(a.tag());

        if ( a.hasValue() )
            (*this) << "=" << a.value();
    }
}

void Printer::visit(Module* m)
{
    setPrintOriginalIDs(false);

    _module = m;

    Printer& p = *this;

    if ( _cfg && ! m->cfg() )
        p << "# No control flow graph available." << endl;

    if ( _cfg && ! m->liveness() )
        p << "# No liveness information available." << endl;

    p << "module " << m->id()->name() << endl;
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

    bool in_function = in<declaration::Function>() || in<declaration::Hook>();

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

//    p << "--- Begin of block: " <<  (b->id() ? b->id()->name() : "-") << endl;
#if 0
    printFlow(b, "--- Begin of block");
    p << endl;

    if ( includeFlow() ) {
        auto cfg = _module->cfg();

        std::set<string> succ;
        for ( auto s : *cfg->successors(b->sharedPtr<Statement>()) )
            succ.insert(_blockName(s.get()));

        std::set<string> pred;
        for ( auto s : *cfg->predecessors(b->sharedPtr<Statement>()) )
            pred.insert(_blockName(s.get()));

        string strp = (pred.size() ? util::strjoin(pred, ", ") : "<none>");
        string strs = (succ.size() ? util::strjoin(succ, ", ") : "<none>");

        p << "# Block predecessors: " << strp << endl;
        p << "# Block successors  : " << strs << endl;
    }
#endif

    for ( auto s : b->statements() )
        p << s << endl;

//   p << "--- End of block: " <<  (b->id() ? b->id()->name() : "-") << endl;

#if 0
    if ( includeFlow() )
        p << "# -- End of block " << _statementName(b) << endl;
#endif

    if ( in_function && parent<statement::Block>() )
        popIndent();
}

void Printer::visit(statement::Try* s)
{
    Printer& p = *this;

    printFlow(s);
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

    printFlow(s);
    p << "for ( " << s->id()->name() << " in " << s->sequence() << " ) {";
    p << endl;
    p << s->body();
    p << "}" << endl;
    p << endl;
}

static void printInstruction(Printer& p, statement::Instruction* i)
{
    if ( i->internal() && ! p.includeFlow() )
        return;

    //  p << util::fmt("# %p\n", i);

    for ( auto c : i->comments() ) {
        if ( c.size() )
            p << "# " << c << endl;
        else
            p << endl;
    }

    p.printFlow(i);

    auto ops = i->operands();

    if ( ops[0] )
        p << ops[0] << " = ";

    string id = util::strreplace(i->id()->name(), ".op.", "");

    p << id;

    for ( int i = 1; i < ops.size(); ++i ) {
        if ( ops[i] )
            p << " " << ops[i];
    }

    p.printMetaInfo(i->metaInfo());
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

    p << scopedID(&p, i, i->id());
}

void Printer::visit(expression::Variable* v)
{
    Printer& p = *this;
    p << scopedID(&p, v, v->variable()->id());
}

void Printer::visit(expression::Parameter* pa)
{
    Printer& p = *this;
    p << pa->parameter()->id();
}

void Printer::visit(expression::Function* f)
{
    Printer& p = *this;
    p << scopedID(&p, f, f->function()->id());
}

void Printer::visit(expression::Module* m)
{
    Printer& p = *this;
    p << scopedID(&p, m, m->module()->id());
}

void Printer::visit(expression::Type* t)
{
    Printer& p = *this;
    auto id = t->type()->id();
    p << (id ? scopedID(&p, t, id) : t->type()->render());
}

void Printer::visit(expression::Default* t)
{
    Printer& p = *this;
    p << t->type()->render() << "()";
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

    bool external = (v->linkage() == Declaration::IMPORTED);
    const char* tag = ast::as<variable::Local>(v->variable()) ? "local" : "global";

    if ( external )
        p << "declare ";

    p << tag << " " << v->variable()->type() << " " << v->id();

    if ( v->variable()->init() && ! external ) {
        p << " = ";
        p << v->variable()->init();
    }

    printAttributes(v->variable()->attributes());
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

    for ( auto c : f->comments() )
        p << "# " << c << endl;

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

    printAttributes(func->type()->attributes());

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

void Printer::visit(type::HiltiFunction * t)
{
    Printer& p = *this;

    switch ( t->callingConvention() ) {
     case type::function::HILTI_C:
        p << "\"C-HILTI\" ";
        break;

     case type::function::C:
        p << "\"C\" ";
        break;

     case type::function::CALLABLE:
        p << "\"CALLABLE\" ";
        break;

     case type::function::HILTI:
     case type::function::HOOK:
        // Default.
        break;

     default:
        internalError("unknown calling convention");
    }

    p << "function(";
    printList(t->parameters(), ", ");
    p << ") -> " << t->result();

    printAttributes(t->attributes());
}

void Printer::visit(type::Hook* t)
{
    Printer& p = *this;
    p << "<hook>";

    printAttributes(t->attributes());
}

void Printer::visit(ID* i)
{
    Printer& p = *this;
    p << i->pathAsString(_module ? _module->id() : nullptr);
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

    printAttributes(i->attributes());
}

void Printer::visit(type::Unknown* i)
{
    Printer& p = *this;
    p << (i->id() ? util::fmt("<Unresolved '%s'>", i->id()->pathAsString().c_str()) : "<Unknown>");
}

void Printer::visit(type::Union* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->wildcard() ) {
        p << "union<*>";
        return;
    }

    if ( t->anonymousFields() ) {
        p << "union<";
        printList(t->typeList(), ", ");
        p << ">";
        printAttributes(t->attributes());
        return;
    }

    if ( ! t->fields().size() ) {
        p << " union { }" << endl;
        printAttributes(t->attributes());
        return;
    }

    p << "union {" << endl;
    pushIndent();
    enableTypeIDs();

    auto i = t->fields().begin();

    while ( i != t->fields().end() ) {
        auto f = *i++;
        auto last = (i == t->fields().end());

        p << f->type() << ' ' << f->id();

        printAttributes(f->attributes());

        if ( ! last )
            p << ",";

        p.printMetaInfo(f->metaInfo());

        p << endl;
    }

    disableTypeIDs();
    popIndent();

    p << "}";
    printAttributes(t->attributes());
    p << endl << endl;
}

void Printer::visit(type::Integer* i)
{
    if ( printTypeID(i) )
        return;

    Printer& p = *this;

    if ( i->wildcard() )
        p << "int<*>";
    else
        p << "int<" << i->width() << ">";

    printAttributes(i->attributes());
}

void Printer::visit(type::String* i)
{
    if ( printTypeID(i) )
        return;

    Printer& p = *this;
    p << "string";

    printAttributes(i->attributes());
}

void Printer::visit(type::Label* i)
{
    if ( printTypeID(i) )
        return;

    Printer& p = *this;
    p << "label";

    printAttributes(i->attributes());
}

void Printer::visit(type::Bool* b)
{
    if ( printTypeID(b) )
        return;

    Printer& p = *this;
    p << "bool";

    printAttributes(b->attributes());
}

void Printer::visit(type::Reference* t)
{
    Printer& p = *this;

    if ( printTypeID(t) )
        return;

    enableTypeIDs();

    if ( t->argType() )
        p << "ref<" << t->argType() << ">";
    else
        p << "ref<*>";

    disableTypeIDs();

    printAttributes(t->attributes());
}

void Printer::visit(type::Exception* e)
{
    if ( printTypeID(e) )
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

    printAttributes(e->attributes());
}

void Printer::visit(type::Bytes* b)
{
    if ( printTypeID(b) )
        return;

    Printer& p = *this;
    p << "bytes";

    printAttributes(b->attributes());
}

void Printer::visit(type::Tuple* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( ! t->wildcard() ) {
        p << "tuple<";
        printList(t->typeList(), ", ");
        p << ">";
    }

    else
        p << "tuple<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::TypeType* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    if ( t->typeType() )
        p << t->typeType();
    else
        p << "(no type)";

    printAttributes(t->attributes());
}

void Printer::visit(type::Address* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "addr";

    printAttributes(c->attributes());
}

void Printer::visit(type::Bitset* c)
{
    if ( printTypeID(c) )
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

    printAttributes(c->attributes());
}

void Printer::visit(type::Scope* s)
{
    if ( printTypeID(s) )
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

    printAttributes(s->attributes());
}

void Printer::visit(type::CAddr* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "caddr";

    printAttributes(c->attributes());
}

void Printer::visit(type::Double* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "double";

    printAttributes(c->attributes());
}

void Printer::visit(type::Enum* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;

    if ( printTypeID(c) )
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

    p << " }";

    printAttributes(c->attributes());
}

void Printer::visit(type::Interval* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "interval";

    printAttributes(c->attributes());
}

void Printer::visit(type::Time* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "time";

    printAttributes(c->attributes());
}

void Printer::visit(type::Network* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "net";

    printAttributes(c->attributes());
}

void Printer::visit(type::Port* c)
{
    if ( printTypeID(c) )
        return;

    Printer& p = *this;
    p << "port";

    printAttributes(c->attributes());
}

void Printer::visit(type::Callable* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->wildcard() ) {
        p << "callable<*>";
        return;
    }

    p << "callable<" << t->result();

    if ( t->Function::parameters().size() ) {
        p << ",";
        printList(t->Function::parameters(), ", ");
    }

    p << ">";

    printAttributes(t->attributes());
}

void Printer::visit(type::Channel* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "channel<" << t->argType() << ">";
    else
        p << "channel<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::Classifier* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( ! t->wildcard() )
        p << "classifier<" << t->ruleType() << "," << t->valueType() << ">";
    else
        p << "classifier<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::File* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    p << "file";

    printAttributes(t->attributes());
}

void Printer::visit(type::IOSource* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->kind() )
        p << "iosrc<" << t->kind() << ">";
    else
        p << "iosrc<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::List* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "list<" << t->argType() << ">";
    else
        p << "list<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::Map* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    assert(! (t->keyType() || t->valueType()) || (t->keyType() && t->valueType()));

    if ( t->keyType() )
        p << "map<" << t->keyType() << ", " << t->valueType() << ">";
    else
        p << "map<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::Vector* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "vector<" << t->argType() << ">";
    else
        p << "vector<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::Set* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "set<" << t->argType() << ">";
    else
        p << "set<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::Overlay* t)
{
    if ( printTypeID(t) )
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

    printAttributes(t->attributes());
}

void Printer::visit(type::OptionalArgument* t)
{
    Printer& p = *this;

    if ( t->argType() )
        p << t->argType();
}

void Printer::visit(type::RegExp* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    p << "regexp";

    printAttributes(t->attributes());
}

void Printer::visit(type::MatchTokenState* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    p << "match_token_state";

    printAttributes(t->attributes());
}

void Printer::visit(type::Struct* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    auto kind = ast::isA<type::Context>(t->sharedPtr<type::Struct>()) ? "context" : "struct";

    if ( ! t->fields().size() ) {
        p << kind << " { }" << endl;
        return;
    }

    p << kind << " {" << endl;
    pushIndent();
    enableTypeIDs();

    auto i = t->fields().begin();

    while ( i != t->fields().end() ) {
#if 0
        for ( auto j = i; ++j != t->fields().end(); ) {
            if ( ! (*j)->internal() )
                last = false;
                break;
        }
#endif

        auto f = *i++;
        auto last = (i == t->fields().end());

#if 0
        if ( f->internal() )
            continue;
#endif

        p << f->type() << ' ' << f->id();

        printAttributes(f->attributes());

        if ( ! last )
            p << ",";

        p.printMetaInfo(f->metaInfo());

        p << endl;
    }

    disableTypeIDs();
    popIndent();

    p << "}";
    printAttributes(t->attributes());
    p << endl << endl;
}

void Printer::visit(type::Timer* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    p << "timer";

    printAttributes(t->attributes());
}

void Printer::visit(type::TimerMgr* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    p << "timer_mgr";

    printAttributes(t->attributes());
}

void Printer::visit(type::Iterator* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;

    if ( t->argType() )
        p << "iterator<" << t->argType() << ">";
    else
        p << "iterator<*>";

    printAttributes(t->attributes());
}

void Printer::visit(type::Unset* t)
{
    if ( printTypeID(t) )
        return;

    Printer& p = *this;
    p << "<type \"unset\">";

    printAttributes(t->attributes());
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

void Printer::visit(constant::CAddr* c)
{
    Printer& p = *this;
    p << "Null"; // Only possible constant.
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

    if ( ! in<constant::Tuple>() ) {
        p << "<<<Unset>>>";
        return;
    }

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
        bits.push_back(scopedID(&p, expr.get(), b));

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
    p << scopedID(&p, expr.get(), c->value());
}

void Printer::visit(constant::Interval* c)
{
    Printer& p = *this;

    p << "interval(" << std::showpoint << std::fixed << (c->value() / 1e9) << ")";
}

void Printer::visit(constant::Time* c)
{
    Printer& p = *this;

    p << "time(" << std::showpoint << std::fixed << (c->value() / 1e9) << ")";
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

     case constant::PortVal::ICMP:
        p << v.port << "/icmp";
        break;

     default:
        internalError("unknown protocol");
    }
}

void Printer::visit(constant::Union* c)
{
    Printer& p = *this;

    p << "union";

    if ( ! c->typeDerived() )
        p << "<" << c->type() << ">";

    p << "(";

    if ( c->id() )
        p << c->id()->name() << ": ";

    if ( c->expression() )
        p << c->expression();

    p << ")";
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
    auto rtype = ast::checkedCast<type::Reference>(c->type());
    auto ktype = ast::checkedCast<type::Map>(rtype->argType())->keyType();
    auto vtype = ast::checkedCast<type::Map>(rtype->argType())->valueType();

    Printer& p = *this;

    p << "map<" << ktype << ", " << vtype << ">(";

    bool first = true;
    for ( auto e: c->elements() ) {
        if ( ! first )
            p << ", ";

        p << e.first << ": " << e.second;

        first = false;
    }

    p << ")";

    printAttributes(c->attributes());
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
    std::list<string> patterns;

    for ( auto p : c->patterns() )
        patterns.push_back(string("/") + p + string("/"));

    printList(patterns, " | ");

    auto rtype = ast::checkedCast<hilti::type::Reference>(c->type())->argType();
    printAttributes(c->attributes());
}

void Printer::visit(ctor::Callable* c)
{
    auto rtype = ast::checkedCast<type::Reference>(c->type());
    auto ctype = ast::checkedCast<type::Callable>(rtype->argType());

    Printer& p = *this;

    p << "callable<" << ctype->result()->type();

    for ( auto a : ctype->Function::parameters() )
        p << ", " << a->type();

    p << ">(" << c->function() << ", (";
    printList(c->arguments(), ", ");
    p << "))";

}
