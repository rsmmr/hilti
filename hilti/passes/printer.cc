
#include "hilti.h"

using namespace hilti::passes;
using namespace hilti::passes::printer;

string Printer::scopedID(Expression* expr, shared_ptr<ID> id)
{
    if ( expr->scope().size() )
        return expr->scope() + "::" + id->pathAsString();
    else
        return id->pathAsString();
}

void Printer::reset()
{
    Visitor<>::reset();
    _indent = 0;
    _bol = true;
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

    for ( auto i : m->exportedIDs() ) {
        // Main::run is implicitly exported, don't print the export.
        if ( i->name() == "run" ) {
            auto m = current<Module>();

            if ( m && util::strtolower(m->id()->name()) == "main" )
                continue;
        }

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

    bool in_function = in<declaration::Function>();

    if ( in_function ) {
        if ( b->id() )
            p << b->id() << ":" << endl;

        pushIndent();
    }

    for ( auto d : b->Declarations() )
        p << d << endl;

    if ( b->Declarations().size() )
        p << endl;

    for ( auto s : b->Statements() )
        p << s << endl;

    if ( in_function )
        popIndent();
}

static void printInstruction(Printer& p, statement::Instruction* i)
{
    auto ops = i->operands();

    if ( ops[0] )
        p << ops[0] << " = ";

    p << i->id();

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

void Printer::visit(expression::Ctor* e)
{
    Printer& p = *this;
    p << e->ctor();
}

void Printer::visit(expression::ID* i)
{
    Printer& p = *this;
    p << i->id();
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
    p << scopedID(t, t->type()->id());
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

void Printer::visit(declaration::Function* f)
{
    Printer& p = *this;

    auto func = f->function();
    auto ftype = func->type();

    bool has_impl = func->body();

    if ( ! has_impl )
        p << "declare ";

    switch ( ftype->callingConvention() ) {
     case type::function::HILTI_C:
        p << "\"C-HILTI\" ";
        break;

     case type::function::C:
        p << "\"C\" ";
        break;

     case type::function::HILTI:
        // Default.
        break;

     default:
        internalError("unknown calling convention");
    }

    p << ftype->result() << " " << f->id() << '(';
    printList(ftype->parameters(), ",");
    p << ')' << endl;

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

    if ( param->constant() )
        p << "const ";

    if ( param->type() )
        p << param->type();

    if ( param->id() )
        p << ' ' << param->id();

    auto def = param->defaultValue();

    if ( def )
        p << '=' << def;
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

void Printer::visit(type::Integer* i)
{
    Printer& p = *this;
    p << "int<" << i->width() << ">";
}

void Printer::visit(type::String* i)
{
    Printer& p = *this;
    p << "string";
}

void Printer::visit(type::Bool* b)
{
    Printer& p = *this;
    p << "bool";
}

void Printer::visit(type::Reference* r)
{
    Printer& p = *this;
    p << "ref<" << r->refType() << ">";
}

void Printer::visit(type::Bytes* b)
{
    Printer& p = *this;
    p << "bytes";
}

void Printer::visit(type::Tuple* t)
{
    Printer& p = *this;

    p << "tuple<";
    printList(t->typeList(), ", ");
    p << ">";
}

void Printer::visit(constant::Integer* i)
{
    Printer& p = *this;
    p << i->value();
}

void Printer::visit(constant::String* s)
{
    Printer& p = *this;
    p << '"' << s->value() << '"';
}

void Printer::visit(constant::Bool* b)
{
    Printer& p = *this;
    p << (b->value() ? "True" : "False");
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
    printList(t->value(), ",");
    p << ')';
}

void Printer::visit(constant::Unset* t)
{
    Printer& p = *this;

    if ( ! in<constant::Tuple>() )
        internalError("printing supports unset value only in tuples");

    p << '*';
}

void Printer::visit(ctor::Bytes* b)
{
    Printer& p = *this;
    p << 'b' << '"' << b->value() << '"';
}

