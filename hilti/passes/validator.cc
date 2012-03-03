
#include "hilti.h"

using namespace hilti::passes;

Validator::~Validator()
{
}

void Validator::wrongType(shared_ptr<Node> node, const string& msg, shared_ptr<Type> have, shared_ptr<Type> want)
{
    return wrongType(node.get(), msg, have, want);
}

void Validator::wrongType(ast::NodeBase* node, const string& msg, shared_ptr<Type> have, shared_ptr<Type> want)
{
    string m = msg;

    if ( have )
        m += string("\n    Type given:    " + have->render());

    if ( want )
        m += string("\n    Type expected: " + want->render());

    return error(node, m);
}

void Validator::visit(Module* m)
{
}

void Validator::visit(ID* id)
{
}

void Validator::visit(statement::Block* s)
{
}

void Validator::visit(statement::Try* s)
{
}

void Validator::visit(statement::instruction::Resolved* s)
{
    s->instruction()->validate(this, s->operands());
}

void Validator::visit(statement::instruction::Unresolved* s)
{
    error(s, "unresolved instruction (should not happen, probably an internal error)");
}

void Validator::visit(statement::try_::Catch* s)
{
    static shared_ptr<Type> refException(new type::Reference(shared_ptr<Type>(new type::Exception())));

    if ( s->type() && s->type() != refException )
        error(s, "exception argument must be of type ref<exception>");
}

void Validator::visit(declaration::Variable* v)
{
}


void Validator::visit(declaration::Function* f)
{
}


void Validator::visit(type::function::Parameter* p)
{
}


void Validator::visit(type::Address* t)
{
}

void Validator::visit(type::Any* t)
{
}

void Validator::visit(type::Bitset* t)
{
}

void Validator::visit(type::Bool* t)
{
}

void Validator::visit(type::Bytes* t)
{
}

void Validator::visit(type::CAddr* t)
{
}

void Validator::visit(type::Callable* t)
{
}

void Validator::visit(type::Channel* t)
{
    if ( ! t->argType() )
        error(t, "no type for channel elements given");

    if ( t->argType() && ! ast::isA<type::ValueType>(t->argType()) )
        error(t, "channel elements must be of value type");
}

void Validator::visit(type::Classifier* t)
{
}

void Validator::visit(type::Double* t)
{
}

void Validator::visit(type::Enum* t)
{
}

void Validator::visit(type::Exception* t)
{
    if ( t->argType() && ! ast::isA<type::ValueType>(t->argType()) )
        error(t, "exception value must be of value type");

    if ( t->baseType() && ! ast::isA<type::Exception>(t->baseType()) )
        error(t, "exception base type must be another exception type");
}

void Validator::visit(type::File* t)
{
}

void Validator::visit(type::Function* t)
{
}

void Validator::visit(type::Hook* t)
{
}

void Validator::visit(type::IOSource* t)
{
    if ( ! t->kind() ) {
        error(t, "no kind for iosrc given");
        return;
    }

    if ( ! util::startsWith(t->kind()->pathAsString(), "Hilti::IOSrc::" ) )
        error(t, util::fmt("iosrc must be of type Hilti::IOSrc::*, but is %s", t->kind()->pathAsString().c_str()));
}

void Validator::visit(type::Integer* t)
{
    auto w = t->width();

    if ( w != 8 && w != 16 && w != 32 && w != 64 )
        error(t, "integer type's width must be 8, 16, 32, or 64.");
}

void Validator::visit(type::Interval* t)
{
}

void Validator::visit(type::Iterator* t)
{
    if ( ! t->argType() )
        error(t, "no type to iterate over given");

    if ( t->argType() && ! type::hasTrait<type::trait::Iterable>(t->argType()) )
        error(t, "iterator over non-iterable type");
}

void Validator::visit(type::List* t)
{
    if ( ! t->argType() )
        error(t, "no type for list elements given");

    if ( t->argType() && ! ast::isA<type::ValueType>(t->argType()) )
        error(t, "list elements must be of value type");
}

void Validator::visit(type::Map* t)
{
    if ( ! t->valueType() )
        error(t, "no type for map elements given");

    if ( ! t->keyType() )
        error(t, "no type for map index given");

    if ( t->valueType() && ! ast::isA<type::ValueType>(t->valueType()) )
        error(t, "map elements must be of value type");

    if ( t->keyType() && ! ast::isA<type::ValueType>(t->keyType()) )
        error(t, "map index must be of value type");

    if ( t->valueType() && ! type::hasTrait<type::trait::Hashable>(t->valueType()) )
        error(t, "map elements must be of hashable type");
}

void Validator::visit(type::MatchTokenState* t)
{
}

void Validator::visit(type::Network* t)
{
}

void Validator::visit(type::Overlay* t)
{
}

void Validator::visit(type::Port* t)
{
}

void Validator::visit(type::Reference* t)
{
    if ( ! t->argType() )
        error(t, "no referenced type given");

    if ( t->argType() && ! ast::isA<type::HeapType>(t->argType()) )
        error(t, "referenced type must be of heap type");
}

void Validator::visit(type::RegExp* t)
{
    for ( auto a : t->attributes() ) {
        if ( a != "&nosub" )
            error(t, util::fmt("unknown regexp attribute '%s'", a.c_str()));
    }
}

void Validator::visit(type::Set* t)
{
    if ( ! t->argType() )
        error(t, "no type for set elements given");

    if ( t->argType() && ! ast::isA<type::ValueType>(t->argType()) )
        error(t, "set elements must be of value type");

    if ( t->argType() && ! type::hasTrait<type::trait::Hashable>(t->argType()) )
        error(t, "set elements must be of hashable type");
}

void Validator::visit(type::String* t)
{
}

void Validator::visit(type::Struct* t)
{
    for ( auto f : t->fields() ) {
        if ( ! f->type() ) {
            error(f, "struct has field without type");
            continue;
        }

        if ( ! f->id() ) {
            error(f, "struct has field without ID");
            continue;
        }

        if ( ! ast::isA<type::ValueType>(f->type()) )
            error(f, "struct fields must be of value type");

        if ( f->default_() && ! f->default_()->canCoerceTo(f->type()) ) {
            wrongType(f->id(), "type of default not compatible with field", f->default_()->type(), f->type());
        }
    }
}

void Validator::visit(type::Time* t)
{
}

void Validator::visit(type::Timer* t)
{
}

void Validator::visit(type::TimerMgr* t)
{
}

void Validator::visit(type::Tuple* t)
{
    for ( auto e : t->typeList() ) {
        if ( ! e ) {
            error(t, "tuple element does not have a type");
            continue;
        }

        if ( ! ast::isA<type::ValueType>(e) )
            error(t, "tuple elements must be of value type");
    }
}

void Validator::visit(type::Type* t)
{
}

void Validator::visit(type::Unknown* t)
{
    internalError(t, util::fmt("unknown type left in AST (%s)", t->render().c_str()));
}

void Validator::visit(type::Vector* t)
{
    if ( ! t->argType() )
        error(t, "no type for vector elements given");

    if ( t->argType() && ! ast::isA<type::ValueType>(t->argType()) )
        error(t, "vector elements must be of value type");
}

void Validator::visit(type::Void* t)
{
}


void Validator::visit(expression::Block* e)
{
}

void Validator::visit(expression::CodeGen* e)
{
}

void Validator::visit(expression::Constant* e)
{
    if ( ! ast::isA<type::ValueType>(e->type()) )
        internalError(e, "constant expression's type should be of value type");
}

void Validator::visit(expression::Ctor* e)
{
    if ( ! ast::isA<type::ValueType>(e->type()) )
        internalError(e, "ctor expression's type should be of heap type");
}

void Validator::visit(expression::Function* e)
{
}

void Validator::visit(expression::ID* e)
{
}

void Validator::visit(expression::Module* e)
{
}

void Validator::visit(expression::Parameter* e)
{
}

void Validator::visit(expression::Type* e)
{
}

void Validator::visit(expression::Variable* e)
{
}

void Validator::visit(constant::Address* c)
{
}

void Validator::visit(constant::Bitset* c)
{
}

void Validator::visit(constant::Bool* c)
{
}

void Validator::visit(constant::Double* c)
{
}

void Validator::visit(constant::Enum* c)
{
}

void Validator::visit(constant::Integer* c)
{
}

void Validator::visit(constant::Interval* c)
{
}

void Validator::visit(constant::Label* c)
{
}

void Validator::visit(constant::Network* c)
{
}

void Validator::visit(constant::Port* c)
{
}

void Validator::visit(constant::Reference* c)
{
}

void Validator::visit(constant::String* c)
{
}

void Validator::visit(constant::Time* c)
{
}

void Validator::visit(constant::Tuple* c)
{
}

void Validator::visit(constant::Unset* c)
{
}

void Validator::visit(ctor::Bytes* c)
{
}

void Validator::visit(ctor::List* c)
{
}

void Validator::visit(ctor::Map* c)
{
}

void Validator::visit(ctor::RegExp* c)
{
}

void Validator::visit(ctor::Set* c)
{
}

void Validator::visit(ctor::Vector* c)
{
}

