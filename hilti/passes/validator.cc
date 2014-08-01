
#include "hilti/hilti-intern.h"
#include "hilti/autogen/instructions.h"

using namespace hilti::passes;

static void _checkReturn(Validator* v, Statement* stmt, shared_ptr<Function> func, shared_ptr<Expression> result)
{
    auto rtype = as<type::Function>(func->type())->result()->type();
    auto is_void = ast::isA<type::Void>(rtype);

    if ( result && is_void ) {
        v->error(stmt, "function does not return a value");
        return;
    }

    if ( ! result && ! is_void ) {
        v->error(stmt, "function must return a value");
        return;
    }

    if ( ! result )
        return;

    if ( ! result->canCoerceTo(rtype) )
        v->wrongType(stmt, "returned type does not match function", result->type(), rtype);
}

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

bool Validator::validReturnType(ast::NodeBase* node, shared_ptr<Type> type, type::function::CallingConvention cc)
{
    if ( ast::isA<type::Void>(type) )
        return true;

    if ( cc != type::function::HILTI && ast::isA<type::Any>(type) )
        return true;

    if ( type::hasTrait<type::trait::ValueType>(type) )
        return true;

    error(node, ::util::fmt("function result must be a value type, but is %s", type->render().c_str()));
    return false;
}


bool Validator::validParameterType(ast::NodeBase* node, shared_ptr<Type> type, type::function::CallingConvention cc)
{
    if ( ast::isA<type::OptionalArgument>(type) )
        type = ast::as<type::OptionalArgument>(type)->argType();

    if ( cc == type::function::HILTI ) {
        auto tuple = ast::as<type::Tuple>(type);
        if ( tuple && tuple->wildcard() ) {
            error(tuple, "HILTI functions cannot have parameter of type tuple<*>");
            return false;
        }
    }

    if ( cc != type::function::HILTI && ast::isA<type::Any>(type) )
        return true;

    if ( type::hasTrait<type::trait::ValueType>(type) )
        return true;

    if ( ast::isA<type::TypeType>(type) )
        return true;

    error(node, ::util::fmt("function parameter must be a value type, but is %s", type->render().c_str()));
    return false;
}

bool Validator::validVariableType(ast::NodeBase* node, shared_ptr<Type> type)
{
    if ( type->wildcard() ) {
        error(node, "cannot create instances of a wildcard type");
        return false;
    }

    if ( type::hasTrait<type::trait::ValueType>(type) )
        return true;

    error(node, ::util::fmt("variable type must be a value type, but is %s", type->render().c_str()));
    return false;
}

void Validator::visit(Module* m)
{
    // Make sure we have a Main::run function.
    if ( util::strtolower(m->id()->name()) == "main" ) {

        auto runs = m->body()->scope()->lookup(std::make_shared<ID>("run"));

        if ( ! runs.size() )
            error(m, "module Main must define a run() function");

        else if ( runs.size() > 1 )
            error(m, "module Main must define only one run() function");

        else {
            auto run = runs.front();

            if ( ! ast::isA<expression::Function>(run) )
                error(m, "in module Main, ID 'run' must be a function");
        }
    }
}

void Validator::visit(ID* id)
{
}

void Validator::visit(statement::Block* s)
{
    for ( auto i : s->scope()->map() ) {
        auto exprs = i.second;

        if ( exprs->size() > 1 ) {
            // This is ok for hooks.
            for ( auto e : *exprs ) {
                auto func = ast::tryCast<expression::Function>(e);

                if ( func && ast::isA<Hook>(func->function()) )
                    continue;

                if ( i.first.find("::") != string::npos )
                    break;

                error(s, util::fmt("ID %s defined more than once", i.first));
                return;
            }
        }
    }
}

void Validator::visit(statement::Try* s)
{
}

void Validator::visit(statement::ForEach* s)
{
    shared_ptr<type::Reference> r = ast::as<type::Reference>(s->sequence()->type());
    shared_ptr<Type> t = r ? r->argType() : s->sequence()->type();

    if ( ! type::hasTrait<type::trait::Iterable>(t) )
        error(s, "expression not iterable");
}

void Validator::visit(statement::instruction::Resolved* s)
{
    pushCurrentLocationNode(s);
    s->instruction()->validate(this, s->operands());
    popCurrentLocationNode();
}

void Validator::visit(statement::instruction::Unresolved* s)
{
    error(s, "unresolved instruction (should not happen, probably an internal error)");
}

void Validator::visit(statement::instruction::flow::ReturnResult* s)
{
    auto decl = current<Declaration>();

    if ( decl && ast::tryCast<declaration::Hook>(decl) ) {
        error(s, "cannot use return.result in a hook; use hook.stop instead");
        return;
    }

    auto func = current<Function>();
    _checkReturn(this, s, func, s->op1());
}

void Validator::visit(statement::instruction::thread::GetContext* s)
{
    auto func = current<Function>();
    assert(func);

    auto ctx = func->module()->executionContext();

    if ( ! ctx )
        error(s, "module does not define an execution context");

#if 0
    // FIXME: This isn't working, seems ctx is picking up the wrong context
    // when used across modules (threads.context-across-modules).
    if ( ! Coercer().canCoerceTo(builder::reference::type(ctx), s->target()->type()) )
        wrongType(s, "invalid target type", s->target()->type(), builder::reference::type(ctx));
#endif
}

void Validator::visit(statement::instruction::thread::SetContext* s)
{
    auto m = current<Module>();
    assert(m);

    auto ctx = m->executionContext();

    if ( ! ctx )
        error(s, "module does not define an execution context");

#if 0
    if ( ! s->op1()->canCoerceTo(builder::reference::type(ctx)) )
        wrongType(s, "invalid operand type", s->op1()->type(), builder::reference::type(ctx));
#endif

    auto func = current<Function>();
    auto scope = func->scope();

    if ( ! scope ) {
        error(s, "current function does not have a scope defined");
        return;
    }

    // TODO: If we get a dynamic struct/context, should we check at runtime
    // that all fields needed by the scope are set?

    auto ttype = ast::as<type::Tuple>(s->op1()->type());

    if ( ttype ) {
        auto c = ast::as<expression::Constant>(s->op1());

        if ( c ) {
            // Make sure we set all fields that our scope requires.
            for ( auto p : util::zip2(ttype->typeList(), ctx->fields()) ) {
                if ( ast::isA<type::Unset>(p.first) && scope->hasField(p.second->id()) ) {
                    error(s, "set of scope fields does not match current function's scope");
                    return;
                }
            }
        }

        else
            // Can't happen I believe.
            internalError("run-time scope type check for dynamic tuples not implemented");
        }

    else {
        // TODO: If we get a dynamic struct/context, should we check at runtime
        // that all fields needed by the scope are set?
    }
}

static void _checkCallScope(Validator* v, statement::Instruction* s)
{
    auto func = v->current<Function>();

    if ( ! func )
        return;

    auto op1 = ast::as<expression::Function>(s->op1());

    if ( ! op1 ) {
        if ( ! ast::isA<type::Function>(s->op1()->type()) )
             v->error(s, "1st operand must reference a function");

        // A non-constant function value. The function that this evaluates to
        // must not have a scope, which we check where we take the value.
        //
        // TODO: Right now, we don't check that actually.
        return;
    }

    auto callee = op1->function();
    assert(callee);
    assert(callee->module());

    auto scope = callee->scope();

    if ( func->scope() ) {
        if ( scope && ! scope->equal(func->scope()) )
            v->error(s, "cannot call function with a different scope; use thread.schedule");
    }
    else {
        if ( scope )
            v->error(s, "cannot call function with scope from one without; use thread.schedule");
    }
}

void Validator::visit(statement::instruction::flow::CallResult* s)
{
    _checkCallScope(this, s);
}

void Validator::visit(statement::instruction::flow::CallVoid* s)
{
    _checkCallScope(this, s);
}

void Validator::visit(statement::instruction::thread::Schedule* s)
{
    if ( ! s->op3() ) {
        // Check whether scope promotion is valid.

        auto func = current<Function>();
        auto module  = current<Module>();

        auto op1 = ast::as<expression::Function>(s->op1());

        if ( ! op1 ) {
            error(s, "1st operand must reference a function");
            return;
        }

        auto callee = op1->function();
        assert(callee);
        assert(callee->module());

        auto src_scope = func->scope();
        auto src_context = module->executionContext();
        auto dst_scope = callee->scope();
        auto dst_context = callee->module()->executionContext();

        if ( ! src_context ) {
            error(s, "current module does not define a thread context");
            return;
        }

        if ( ! dst_context ) {
            error(s, "callee's module does not define a thread context");
            return;
        }

        if ( ! src_scope ) {
            error(s, "current function does not define a scope");
            return;
        }

        if ( ! dst_scope ) {
            error(s, "callee function does not define a scope");
            return;
        }

        // For a valid promotion, all fields of the destination context must
        // also be defined in the source scope (i.e., match in name and
        // type).

        for ( auto dst_field : dst_context->fields() ) {

            if ( ! dst_scope->hasField(dst_field->id()) )
                // Don't need this field.
                continue;

            if ( ! src_scope->hasField(dst_field->id()) )
                goto mismatch;

            auto src_field = src_context->lookup(dst_field->id());

            if ( ! src_field )
                // Field missing.
                goto mismatch;

            if ( ! dst_field->type()->equal(src_field->type()) )
                // Type mismatch.
                goto mismatch;

        }
    }

    return;

mismatch:
    error(s, "scope of callee function is incompatible with the current scope");
}

void Validator::visit(statement::instruction::flow::ReturnVoid* s)
{
    if ( current<Hook>() )
        return;

    auto func = current<Function>();
    _checkReturn(this, s, func, nullptr);
}

void Validator::visit(statement::try_::Catch* s)
{
    static shared_ptr<Type> refException(new type::Reference(shared_ptr<Type>(new type::Exception())));

    if ( s->type() && ! s->type()->equal(refException) )
        error(s, "exception argument must be of type ref<exception>");
}

void Validator::visit(declaration::Variable* v)
{
    validVariableType(v, v->variable()->type());
}

void Validator::visit(declaration::Function* f)
{
    auto func = f->function();

    if ( func->initFunction() ) {
        if ( ! ast::isA<type::Void>(func->type()->result()->type()) )
            error(f, "init function cannot return a value");

        if ( func->type()->parameters().size() )
            error(f, "init function cannot take parameters");
    }
}

void Validator::visit(declaration::Type* t)
{
    if ( t->id()->name() == "Context" && ! ast::isA<type::Context>(t->type()) )
        error(t, "ID 'Context' is reserved for the module's execution context");

    auto scope = ast::as<type::Scope>(t->type());

    if ( scope ) {
        auto context = current<Module>()->executionContext();

        if ( ! context ) {
            error(t, "module does not define a context");
            return;
        }

        for ( auto f : scope->fields() ) {
            if ( ! context->lookup(f) )
                error(t, "scope uses field that context does not define");
        }
    }
}

void Validator::visit(declaration::Hook* f)
{
    auto block = current<statement::Block>();
    assert(block);

    auto other = block->scope()->lookup(f->id());

#if 0
    // Allow to define external hooks without declaring them first. That
    // allows for declaring hooks without importing the other module.
    if ( f->id()->isScoped() && ! other.size() )
        error(f, util::fmt("external hook %s not declared", f->id()->pathAsString().c_str()));
#endif

    if ( other.size() && ! f->hook()->type()->equal(other.front()->type()) )
        error(f, util::fmt("inconsistent definitions for hook %s", f->id()->pathAsString().c_str()));
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
    if ( t->wildcard() )
        return;

    if ( ! t->argType() )
        error(t, "no type for channel elements given");

    if ( t->argType() && ! type::hasTrait<type::trait::ValueType>(t->argType()) )
        error(t, "channel elements must be of value type");
}

void Validator::visit(type::Classifier* t)
{
    if ( t->wildcard() )
        return;

    if ( ! t->ruleType() )
        error(t, "no type for classifier rules given");

    if ( ! t->valueType() )
        error(t, "no type for classifier values given");

    if ( t->ruleType() ) {
        if ( ! ast::isA<type::Struct>(t->ruleType()) )
            error(t, "rule type must be a struct");

        else {
            for ( auto l : ast::as<type::Struct>(t->ruleType())->typeList() ) {
                auto r = ast::as<type::Reference>(l);
                auto t = r ? r->argType() : l;

                if ( ! type::hasTrait<type::trait::Classifiable>(t) )
                    error(l, "type cannot be used in a classifier");
            }
        }
    }
}

void Validator::visit(type::Double* t)
{
}

void Validator::visit(type::Enum* t)
{
}

void Validator::visit(type::Exception* t)
{
    if ( t->argType() && ! type::hasTrait<type::trait::ValueType>(t->argType()) )
        error(t, "exception argument type must be of value type");

    auto btype = ast::as<type::Exception>(t->baseType());

    if ( t->baseType() && ! btype ) {
        error(t, "exception base type must be another exception type");
        return;
    }

    if ( t->baseType() && btype->argType() && ! t->argType() ) {
        error(t, ::util::fmt("exception type must have same argument type as its parent, which has %s", btype->argType()->render().c_str()));
        return;
    }

    if ( t->argType() && t->baseType() && ! btype->argType() ) {
        error(t, "exception type must not have an argument type because its parent type does not either");
        return;
    }

    if ( t->argType() && t->baseType() && btype->argType() && ! t->argType()->equal(btype->argType()) ) {
        error(t, ::util::fmt("exception type must have same argument type as its parent type, which has %s", btype->argType()->render().c_str()));
        return;
    }

    // Check for cycles in inheritance chain.
    std::set<shared_ptr<hilti::Type>> seen;

    for ( shared_ptr<hilti::Type> c = t->sharedPtr<hilti::Type>(); c; c = ast::as<type::Exception>(c)->baseType() ) {
        for ( auto o : seen ) {
            if ( c == o  ) { // Really a pointer comparision here, no structural equivalence.
                // Break the cycle here so that printing works. Then abort
                // AST visiting right here.
                t->setBaseType(nullptr);
                fatalError(t, "circuluar exception inheritance");
            }
        }

        seen.insert(c);
    }
}

void Validator::visit(type::File* t)
{
}

void Validator::visit(type::Function* t)
{
    if ( ! validReturnType(t, t->result()->type(), t->callingConvention()) )
        return;

    for ( auto p : t->parameters() ) {
        if ( ! validParameterType(t, p->type(), t->callingConvention()) )
            continue;
    }
}

void Validator::visit(type::Hook* t)
{
}

void Validator::visit(type::IOSource* t)
{
    if ( t->wildcard() )
        return;

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
    if ( ! t->argType() && ! in<type::HiltiFunction>() )
        error(t, "no type for list elements given");

    if ( t->argType() && ! type::hasTrait<type::trait::ValueType>(t->argType()) )
        error(t, "list elements must be of value type");
}

void Validator::visit(type::Map* t)
{
    if ( ! t->valueType() && ! in<type::HiltiFunction>() )
        error(t, "no type for map elements given");

    if ( ! t->keyType() && ! in<type::HiltiFunction>() )
        error(t, "no type for map index given");

    if ( t->valueType() && ! type::hasTrait<type::trait::ValueType>(t->valueType()) )
        error(t, "map elements must be of value type");

    if ( t->keyType() && ! type::hasTrait<type::trait::ValueType>(t->keyType()) )
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

void Validator::visit(type::overlay::Field* f)
{
    if ( f->startOffset() >= 0 && f->startField() )
        error(f, "field cannot specify both offset and preceeding field");

    if ( ! (f->startOffset() >= 0 || f->startField()) )
        error(f, "field must specify either offset pr preceeding field");

    if ( ! type::hasTrait<type::trait::Unpackable>(f->type()) )
        error(f, "field type does not support unpacking");
}

void Validator::visit(type::Overlay* t)
{
    std::set<string> names;

    for ( auto f : t->fields() ) {
        if ( names.find(f->name()->name()) != names.end() )
            error(f, "field name already defined");

        if ( f->startField() && ! t->field(f->startField()->name()) )
            error(f, "dependent field must be defined first");
    }
}

void Validator::visit(type::Port* t)
{
}

void Validator::visit(type::Reference* t)
{
    if ( ! t->argType() )
        error(t, "no referenced type given");

    if ( t->argType() && ! type::hasTrait<type::trait::HeapType>(t->argType()) )
        error(t, "referenced type must be of heap type");
}

void Validator::visit(type::RegExp* t)
{
    for ( auto a : t->attributes() ) {
        if ( a != "&nosub" && a != "&first_match" )
            error(t, util::fmt("unknown regexp attribute '%s'", a.c_str()));
    }
}

void Validator::visit(type::Set* t)
{
    if ( ! t->argType() && ! in<type::HiltiFunction>() )
        error(t, "no type for set elements given");

    if ( t->argType() && ! type::hasTrait<type::trait::ValueType>(t->argType()) )
        error(t, "set elements must be of value type");

    if ( t->argType() && ! type::hasTrait<type::trait::Hashable>(t->argType()) )
        error(t, "set elements must be of hashable type");
}

void Validator::visit(type::String* t)
{
}

void Validator::visit(type::Struct* t)
{
    std::set<string> names;

    for ( auto f : t->fields() ) {

        if ( names.find(f->id()->name()) != names.end() ) {
            error(f, "duplicate field name in struct");
            return;
        }

        names.insert(f->id()->name());

        if ( ! f->type() ) {
            error(f, "struct has field without type");
            continue;
        }

        if ( ! f->id() ) {
            error(f, "struct has field without ID");
            continue;
        }

        if ( ! type::hasTrait<type::trait::ValueType>(f->type()) )
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

        if ( ! type::hasTrait<type::trait::ValueType>(e) )
            error(t, "tuple elements must be of value type");
    }
}

void Validator::visit(type::TypeType* t)
{
}

void Validator::visit(type::Unknown* t)
{
    internalError(t, util::fmt("unknown type left in AST (%s)", t->render().c_str()));
}

void Validator::visit(type::Vector* t)
{
    if ( ! t->argType() && ! in<type::HiltiFunction>() )
        error(t, "no type for vector elements given");

    if ( t->argType() && ! type::hasTrait<type::trait::ValueType>(t->argType()) )
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
    if ( ast::isA<type::Label>(e->type()) )
        return;

    if ( ! type::hasTrait<type::trait::ValueType>(e->type()) )
        internalError(e, "constant expression's type needs to be of value type");
}

void Validator::visit(expression::Ctor* e)
{
    if ( ! type::hasTrait<type::trait::ValueType>(e->type()) )
        internalError(e, "ctor expression's type needs to be of value type");
}

void Validator::visit(expression::Default* e)
{
    if ( ! type::hasTrait<type::trait::ValueType>(e->type()) )
        error(e, "constructor's type needs to be of value type");
}

void Validator::visit(expression::Function* e)
{
    auto func = e->function();

    if ( ! (*func->module()->id() == *current<Module>()->id()) &&
         ! func->module()->exported(func->id()) )
        error(e, "calling function from external module that is not exported");
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

void Validator::visit(expression::Void* e)
{
    error(e, "void expression encountered that should never be used");
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

void Validator::visit(constant::CAddr* c)
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

void Validator::visit(ctor::Callable* c)
{
}
