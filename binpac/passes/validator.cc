
#include "validator.h"
#include "../attribute.h"
#include "../declaration.h"
#include "../expression.h"
#include "../grammar.h"
#include "../scope.h"
#include "../statement.h"
#include "../type.h"

using namespace binpac;
using namespace binpac::passes;

Validator::Validator() : Pass<AstInfo>("binpac::Validator", false)
{
}

bool Validator::run(shared_ptr<ast::NodeBase> node)
{
    return processAllPreOrder(node);
}

Validator::~Validator()
{
}

void Validator::wrongType(ast::NodeBase* node, const string& msg, shared_ptr<Type> have,
                          shared_ptr<Type> want)
{
    string m = msg;

    if ( have )
        m += string("\n    Type given:    " + have->render());

    if ( want )
        m += string("\n    Type expected: " + want->render());

    return error(node, m);
}

void Validator::wrongType(shared_ptr<Node> node, const string& msg, shared_ptr<Type> have,
                          shared_ptr<Type> want)
{
    return wrongType(node.get(), msg, have, want);
}

bool Validator::checkReturnType(ast::NodeBase* node, shared_ptr<Type> type)
{
    // TODO.
    return true;
}

bool Validator::checkParameterType(ast::NodeBase* node, shared_ptr<Type> type)
{
    // TODO.
    return true;
}

bool Validator::checkVariableType(ast::NodeBase* node, shared_ptr<Type> type)
{
    // TODO.
    return true;
}

void Validator::visit(Attribute* a)
{
}

void Validator::visit(AttributeSet* a)
{
}

void Validator::visit(Constant* c)
{
}

void Validator::visit(Ctor* c)
{
}

void Validator::visit(Declaration* d)
{
}

void Validator::visit(Expression* e)
{
}

void Validator::visit(Function* f)
{
}

void Validator::visit(Hook* h)
{
}

void Validator::visit(ID* i)
{
}

void Validator::visit(Module* m)
{
}

void Validator::visit(Statement* s)
{
}

void Validator::visit(Variable* v)
{
}

void Validator::visit(constant::Address* a)
{
}

void Validator::visit(constant::Bitset* b)
{
}

void Validator::visit(constant::Bool* b)
{
}

void Validator::visit(constant::Double* d)
{
}

void Validator::visit(constant::Enum* e)
{
}

#if 0
void Validator::visit(constant::Expression* e)
{
}
#endif

void Validator::visit(constant::Integer* i)
{
}

void Validator::visit(constant::Interval* i)
{
}

void Validator::visit(constant::Network* n)
{
}

void Validator::visit(constant::Port* p)
{
}

void Validator::visit(constant::String* s)
{
}

void Validator::visit(constant::Time* t)
{
}

void Validator::visit(constant::Tuple* t)
{
}

void Validator::visit(ctor::Bytes* b)
{
}

void Validator::visit(ctor::List* l)
{
}

void Validator::visit(ctor::Map* m)
{
}

void Validator::visit(ctor::RegExp* r)
{
}

void Validator::visit(ctor::Set* s)
{
}

void Validator::visit(ctor::Unit* u)
{
    int cnt = 0;

    auto utype = ast::checkedCast<type::Unit>(u->type());

    for ( auto i : utype->items() ) {
        if ( i->ctorNoName() )
            ++cnt;
    }

    if ( cnt > 0 && cnt < u->items().size() )
        error(u, "in union ctor either all fields or none must have a name");
}

void Validator::visit(ctor::Vector* v)
{
}

void Validator::visit(declaration::Constant* c)
{
}

void Validator::visit(declaration::Function* f)
{
}

void Validator::visit(declaration::Hook* h)
{
}

void Validator::visit(declaration::Type* t)
{
    auto unit = ast::tryCast<type::Unit>(t->type());

    if ( unit ) {
// TODO: It's ok to have the same field name multiple times under
// certain constraints: it's ok if the fields using the name are on
// the same "level", i.e., all inside the top unit, or inside cases
// of the same switch. It's not ok to be on different levels, or
// within separate switches (as that will be messed up due to the
// unions used for compiling it).
#if 0
        std::set<string> have;

        for ( auto f : unit->flattenedFields() ) {
            if ( have.find(f->id()->name()) != have.end() )
                error(f, ::util::fmt("duplicate field name '%s'", f->id()->name()));

            have.insert(f->id()->name());
        }
#endif
    }

    if ( unit && unit->grammar() ) {
        string err = unit->grammar()->check();

        if ( err.size() )
            error(unit, err);
    }
}

void Validator::visit(declaration::Variable* v)
{
}

void Validator::visit(expression::Assign* a)
{
    if ( ! a->source()->canCoerceTo(a->destination()->type()) ) {
        error(a->source(), "incompatible type for assigment");
    }
}

void Validator::visit(expression::CodeGen* c)
{
}

void Validator::visit(expression::Coerced* c)
{
}

void Validator::visit(expression::Constant* c)
{
}

void Validator::visit(expression::Conditional* c)
{
    if ( ! c->condition()->canCoerceTo(std::make_shared<type::Bool>()) )
        error(c->condition(), "condition must be a boolean value");

    if ( ! c->false_()->canCoerceTo(c->true_()->type()) )
        error(c, "conditional alternatives have incompatible types");
}

void Validator::visit(expression::Ctor* c)
{
}

void Validator::visit(expression::Function* f)
{
}

void Validator::visit(expression::ID* i)
{
}

void Validator::visit(expression::List* l)
{
}

void Validator::visit(expression::ListComprehension* c)
{
    if ( ! ast::type::trait::hasTrait<type::trait::Iterable>(c->input()->type()) )
        error(c->input(), "non-iterable gives as input set for list comprehension");
}

void Validator::visit(expression::Module* m)
{
}

void Validator::visit(expression::Parameter* p)
{
}

void Validator::visit(expression::ParserState* p)
{
}

void Validator::visit(expression::ResolvedOperator* r)
{
    auto op = r->operator_();
    op->validate(this, nullptr, r->operands());
}

void Validator::visit(expression::Type* t)
{
}

void Validator::visit(expression::UnresolvedOperator* u)
{
    error(u, "operator left unresolved");
}

void Validator::visit(expression::Variable* v)
{
}

void Validator::visit(statement::Block* b)
{
    for ( auto i : b->scope()->map() ) {
        auto exprs = i.second;

        if ( exprs->size() <= 1 )
            continue;

        for ( auto e : *exprs ) {
            // Only functions can be overloaded.
            if ( ! ast::isA<expression::Function>(e) ) {
                error(b, util::fmt("ID %s defined more than once", i.first));
                break;
            }
        }
    }
}

void Validator::visit(statement::Expression* e)
{
}

void Validator::visit(statement::ForEach* f)
{
}

void Validator::visit(statement::IfElse* i)
{
}

void Validator::visit(statement::NoOp* n)
{
}

void Validator::visit(statement::Print* p)
{
}

void Validator::visit(statement::Return* r)
{
}

void Validator::visit(statement::Try* t)
{
}

void Validator::visit(statement::try_::Catch* c)
{
}

void Validator::visit(Type* t)
{
}

void Validator::visit(type::Address* a)
{
}

void Validator::visit(type::Any* a)
{
}

void Validator::visit(type::Bitfield* bm)
{
    auto w = bm->width();

    if ( bm->bits().size() ) {
        if ( w != 8 && w != 16 && w != 32 && w != 64 )
            error(bm, "bitfield's width must be 8, 16, 32, or 64");
    }

    for ( auto b : bm->bits() ) {
        if ( b->lower() > b->upper() || b->lower() < 0 || b->upper() >= w )
            error(b, "invalid bit range");
    }
}

void Validator::visit(type::Bitset* b)
{
}

void Validator::visit(type::Block* b)
{
}

void Validator::visit(type::Bool* b)
{
}

void Validator::visit(type::Bytes* b)
{
}

void Validator::visit(type::CAddr* c)
{
}

void Validator::visit(type::Double* d)
{
}

void Validator::visit(type::Enum* e)
{
}

void Validator::visit(type::Exception* e)
{
}

void Validator::visit(type::File* f)
{
}

void Validator::visit(type::Function* f)
{
}

void Validator::visit(type::Hook* h)
{
}

void Validator::visit(type::Integer* i)
{
}

void Validator::visit(type::Interval* i)
{
}

void Validator::visit(type::Iterator* i)
{
}

void Validator::visit(type::List* l)
{
}

void Validator::visit(type::Map* m)
{
}

void Validator::visit(type::Module* m)
{
}

void Validator::visit(type::Network* n)
{
}

void Validator::visit(type::OptionalArgument* o)
{
}

void Validator::visit(type::Port* p)
{
}

void Validator::visit(type::RegExp* r)
{
}

void Validator::visit(type::Set* s)
{
}

void Validator::visit(type::Sink* s)
{
}

void Validator::visit(type::String* s)
{
}

void Validator::visit(type::Time* t)
{
}

void Validator::visit(type::Tuple* t)
{
}

void Validator::visit(type::TypeByName* t)
{
}

void Validator::visit(type::TypeType* t)
{
}

void Validator::visit(type::Unit* u)
{
}

void Validator::visit(type::Unknown* u)
{
}

void Validator::visit(type::Unset* u)
{
}

void Validator::visit(type::Vector* v)
{
}

void Validator::visit(type::Void* v)
{
}

void Validator::visit(type::function::Parameter* p)
{
}

void Validator::visit(type::function::Result* r)
{
}

void Validator::visit(type::iterator::Bytes* b)
{
}

void Validator::visit(type::iterator::List* l)
{
}

void Validator::visit(type::iterator::Regexp* r)
{
}

void Validator::visit(type::iterator::Set* s)
{
}

void Validator::visit(type::iterator::Vector* v)
{
}

void Validator::visit(type::unit::Item* i)
{
}

void Validator::visit(type::unit::item::Field* f)
{
    auto attributes = f->attributes();
    auto parseable = ast::type::tryTrait<type::trait::Parseable>(f->type());

    if ( ! parseable ) {
        error(f, "type cannot be used in unit field");
        return;
    }

    // See if all attributes are known.

    for ( auto attr : attributes->attributes() ) {
        if ( attr->key() == "default" )
            continue;

        if ( attr->key() == "convert" )
            continue;

        if ( attr->key() == "convert_back" )
            continue;

        if ( attr->key() == "parse" )
            continue;

        if ( attr->key() == "transient" )
            continue;

        if ( attr->key() == "synchronize" )
            continue;

        if ( attr->key() == "length" )
            continue;

        if ( attr->key() == "try" )
            continue;

        if ( attr->key() == "hide" )
            continue;

        bool found = false;

        for ( auto pattr : parseable->parseAttributes() ) {
            if ( pattr.key == attr->key() )
                found = true;
        }

        if ( ! found )
            error(f, util::fmt("unsupported attribute %s", attr->key()));
    }

    // Check common attributes.

    auto attr = attributes->lookup("default");

    if ( attr ) {
        if ( ! attr->value() ) {
            error(attr, "&default attribute needs an expression");
            return;
        }

        if ( ! attr->value()->canCoerceTo(f->type()) ) {
            error(attr, "&default value's type does not match");
            return;
        }
    }

    attr = attributes->lookup("convert");

    if ( attr ) {
        if ( ! attr->value() ) {
            error(attr, "&convert attribute needs an expression");
            return;
        }

        if ( ! ast::isA<type::PacType>(attr->value()->type()) ) {
            error(attr, "invalid type for &convert's expression");
            return;
        }
    }

    attr = attributes->lookup("transient");

    if ( attr && attr->value() ) {
        error(attr, "&transient does not take a value");
        return;
    }

    // Check the type-specific attributes.
    for ( auto pattr : parseable->parseAttributes() ) {
        auto attr = attributes->lookup(pattr.key);

        if ( ! attr )
            return;

        if ( attr->value() && ! pattr.type ) {
            error(attr, "attribute does not take an expression");
            continue;
        }

        if ( pattr.type && ! attr->value() && ! pattr.default_ ) {
            error(attr, "attribute requires an expression");
            continue;
        }

        auto ptype = pattr.type;

        // Resolve TypeByName's manually here.
        auto tbn = ast::tryCast<type::TypeByName>(ptype);

        if ( tbn ) {
            auto module = current<Module>();
            assert(module);
            auto t = module->body()->scope()->lookupUnique(tbn->id());
            assert(t);
            ptype = ast::checkedCast<expression::Type>(t)->typeValue();
        }

        if ( attr->value() && ! attr->value()->canCoerceTo(ptype) ) {
            wrongType(attr->value(), "attribute value's type does not match", attr->value()->type(),
                      ptype);
            continue;
        }
    }
}

void Validator::visit(type::unit::item::GlobalHook* g)
{
}

void Validator::visit(type::unit::item::Property* p)
{
    auto prop = p->property();

    if ( ::util::startsWith(prop->key(), "skip-") ) {
        if ( prop->value() &&
             ! ast::type::trait::hasTrait<type::trait::Parseable>(prop->value()->type()) )
            error(p, "skip expression is not of parseable type");
    }
}

void Validator::visit(type::unit::item::Variable* v)
{
}

void Validator::visit(type::unit::item::field::Container* f)
{
}

void Validator::visit(type::unit::item::field::container::List* f)
{
}

void Validator::visit(type::unit::item::field::container::Vector* f)
{
}

void Validator::visit(type::unit::item::field::Constant* c)
{
}

void Validator::visit(type::unit::item::field::Ctor* t)
{
}

void Validator::visit(type::unit::item::field::Switch* s)
{
    if ( s->cases().empty() ) {
        error(s, "switch without cases");
        return;
    }

    int defaults = 0;

    bool have_expr = (s->expression() != nullptr);

    std::set<string> have;

    for ( auto c : s->cases() ) {
        if ( c->fields().empty() ) {
            error(c, "switch case without any item");
        }

        if ( c->default_() )
            ++defaults;

        if ( have_expr && ! c->default_() && c->expressions().empty() ) {
            error(c, "case without expression");
            break;
        }

        if ( ! have_expr && c->expressions().size() ) {
            error(c, "case does not expect expression");
            break;
        }

        for ( auto e : c->expressions() ) {
            if ( ! e->canCoerceTo(s->expression()->type()) ) {
                error(e, "case value does not match switch expression");
                break;
            }

            auto render = e->render();

            if ( have.find(render) != have.end() )
                error(e, "duplicate case");

            have.insert(render);
        }
    }

    if ( defaults > 1 )
        error(s, "more than one default case");
}

void Validator::visit(type::unit::item::field::AtomicType* t)
{
}

void Validator::visit(type::unit::item::field::Unit* t)
{
}

void Validator::visit(type::unit::item::field::Unknown* f)
{
}

void Validator::visit(type::unit::item::field::switch_::Case* c)
{
}

void Validator::visit(variable::Global* g)
{
}

void Validator::visit(variable::Local* l)
{
}
