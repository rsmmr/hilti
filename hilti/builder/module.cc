
#include "builder/module.h"
#include "hilti-intern.h"

namespace hilti {
namespace builder {

ModuleBuilder::ModuleBuilder(shared_ptr<ID> id, const std::string& path, const path_list& libdirs, const Location& l)
{
    Init(id, path, libdirs, l);
}

ModuleBuilder::ModuleBuilder(const std::string& id, const std::string& path, const path_list& libdirs, const Location& l)
{
    Init(std::make_shared<ID>(id, l), path, libdirs, l);
}

void ModuleBuilder::Init(shared_ptr<ID> id, const std::string& path, const path_list& libdirs, const Location& l)
{
    _module = builder::module::create(id, path, l);
    _libdirs = libdirs;

    // Push a top-level function state as a place-holder for the module's statement.
    auto func = std::make_shared<ModuleBuilder::Function>();
    func->function = nullptr;
    func->location = l;
    _functions.push_back(func);

    // Push a top level body.
    auto body = std::make_shared<ModuleBuilder::Body>();
    body->stmt = std::make_shared<statement::Block>(nullptr, nullptr, l);
    body->scope = body->stmt->scope();

    auto builder = shared_ptr<BlockBuilder>(new BlockBuilder(body->stmt, body->stmt, this));  // No need for two level of blocks here.
    body->builders.push_back(builder);

    func->bodies.push_back(body);
    _module->setBody(body->stmt);
}

ModuleBuilder::~ModuleBuilder()
{
}

bool ModuleBuilder::finalize(bool resolve, bool validate)
{
    _finalized = true;

    if ( errors() )
        return false;

    if ( resolve ) {
        if ( ! hilti::resolveAST(_module, _libdirs) )
            return false;
    }

    if ( validate ) {
        if ( ! hilti::validateAST(_module) )
            return false;
    }

    _correct = true;

    return true;
}

void ModuleBuilder::print(std::ostream& out)
{
    if ( ! _finalized )
        internalError("ModuleBuilder::print() called before ModuleBuilder::finalize()");

    if ( ! _correct )
        internalError("ModuleBuilder::print() called after error in ModuleBuilder::finalize()");

    hilti::printAST(_module, out);
}

shared_ptr<ModuleBuilder::Function> ModuleBuilder::_currentFunction() const
{
    assert(_functions.size());
    return _functions.back();
}

shared_ptr<ModuleBuilder::Body> ModuleBuilder::_currentBody() const
{
    auto func = _currentFunction();
    assert(func->bodies.size());
    return func->bodies.back();
}

shared_ptr<BlockBuilder> ModuleBuilder::_currentBuilder() const
{
    auto body = _currentBody();
    assert(body->builders.size());
    return body->builders.back();
}

void ModuleBuilder::exportType(shared_ptr<hilti::Type> type)
{
    _module->exportType(type);
}


void ModuleBuilder::exportID(shared_ptr<hilti::ID> type)
{
    _module->exportID(type);
}


shared_ptr<ID> ModuleBuilder::uniqueID(shared_ptr<ID> id, shared_ptr<Scope> scope, bool force_unique, bool global)
{
    if ( ! scope )
        scope = _module->body()->scope();

    int i = 1;
    shared_ptr<ID> uid = id;

    while ( scope->lookup(uid, false) ) {

        if ( ! force_unique ) {
            error(::util::fmt("ModuleBuilder: ID %s already defined", uid->name().c_str()), id);
            return std::make_shared<ID>("<error>");
        }

        std::string s = ::util::fmt("%s.%d", id->name().c_str(), ++i);
        uid = std::make_shared<ID>(s, uid->location());
    }

    return uid;
}

shared_ptr<hilti::expression::Function> ModuleBuilder::pushFunction(shared_ptr<hilti::Function> function, bool no_body)
{
    auto func = std::make_shared<ModuleBuilder::Function>();
    func->function = function;
    func->location = function->location();
    _functions.push_back(func);

    auto decl = std::make_shared<declaration::Function>(function, function->location());
    _module->body()->addDeclaration(decl);

    if ( ! no_body )
        pushBody();

    return std::make_shared<hilti::expression::Function>(function);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::declareFunction(shared_ptr<hilti::Function> func)
{
    auto decl = std::make_shared<declaration::Function>(func, func->location());
    _module->body()->addDeclaration(decl);
    return std::make_shared<hilti::expression::Function>(func);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::pushFunction(shared_ptr<ID> id,
                                                              shared_ptr<hilti::function::Parameter> result,
                                                              const hilti::function::parameter_list& params,
                                                              hilti::type::function::CallingConvention cc,
                                                              bool no_body,
                                                              const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Parameter>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Function>(result, params, cc, l);
    auto func = std::make_shared<hilti::Function>(id, ftype, _module, nullptr, l);
    return pushFunction(func, no_body);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::pushFunction(const std::string& id,
                                                              shared_ptr<hilti::function::Parameter> result,
                                                              const hilti::function::parameter_list& params,
                                                              hilti::type::function::CallingConvention cc,
                                                              bool no_body,
                                                              const Location& l)
{
    return pushFunction(std::make_shared<ID>(id, l), result, params, cc, no_body, l);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::declareHook(shared_ptr<hilti::Hook> hook)
{
    return declareFunction(hook);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::declareFunction(shared_ptr<ID> id,
                                                                       shared_ptr<hilti::function::Parameter> result,
                                                                       const hilti::function::parameter_list& params,
                                                                       hilti::type::function::CallingConvention cc,
                                                                       const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Parameter>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Function>(result, params, cc, l);
    auto func = std::make_shared<hilti::Function>(id, ftype, _module, nullptr, l);
    return declareFunction(func);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::declareFunction(const std::string& id,
                                                                       shared_ptr<hilti::function::Parameter> result,
                                                                       const hilti::function::parameter_list& params,
                                                                       hilti::type::function::CallingConvention cc,
                                                                       const Location& l)
{
    return declareFunction(std::make_shared<ID>(id, l), result, params, cc, l);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::declareHook(shared_ptr<ID> id,
                                                                   shared_ptr<hilti::function::Parameter> result,
                                                                   const hilti::function::parameter_list& params,
                                                                   const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Parameter>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    auto func = std::make_shared<hilti::Hook>(id, ftype, _module, hilti::hook::attribute_list(), nullptr, l);
    return declareHook(func);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::declareHook(const std::string& id,
                                                                   shared_ptr<hilti::function::Parameter> result,
                                                                   const hilti::function::parameter_list& params,
                                                                   const Location& l)
{
    return declareHook(id, result, params, l);
}


shared_ptr<hilti::expression::Function> ModuleBuilder::pushHook(shared_ptr<ID> id,
                                                          shared_ptr<hilti::function::Parameter> result,
                                                          const hilti::function::parameter_list& params,
                                                          const hilti::hook::attribute_list& attrs,
                                                          bool no_body,
                                                          const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Parameter>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    auto func =  std::make_shared<hilti::Hook>(id, ftype, _module, attrs, nullptr, l);
    return pushFunction(func, no_body);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::pushHook(const std::string& id,
                                                          shared_ptr<hilti::function::Parameter> result,
                                                          const hilti::function::parameter_list& params,
                                                          const hilti::hook::attribute_list& attrs,
                                                          bool no_body,
                                                          const Location& l)
{
    return pushHook(std::make_shared<ID>(id, l), result, params, attrs, no_body, l);

}

shared_ptr<hilti::expression::Function> ModuleBuilder::pushHook(shared_ptr<ID> id,
                                                          shared_ptr<hilti::function::Parameter> result,
                                                          const hilti::function::parameter_list& params,
                                                          int64_t priority, int64_t group,
                                                          bool no_body,
                                                          const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Parameter>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    auto func = std::make_shared<hilti::Hook>(id, ftype, _module, priority, group, nullptr, l);
    return pushFunction(func, no_body);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::pushHook(const std::string& id,
                                                          shared_ptr<hilti::function::Parameter> result,
                                                          const hilti::function::parameter_list& params,
                                                          int64_t priority, int64_t group,
                                                          bool no_body,
                                                          const Location& l)
{
    return pushHook(std::make_shared<ID>(id, l), result, params, priority, group, no_body, l);
}

shared_ptr<hilti::expression::Function> ModuleBuilder::popFunction()
{
    auto func = _functions.back()->function;
    _functions.pop_back();
    return std::make_shared<hilti::expression::Function>(func, func->location());
}

shared_ptr<hilti::expression::Function> ModuleBuilder::popHook()
{
    return popFunction();
}

shared_ptr<BlockBuilder> ModuleBuilder::pushBody(bool no_builder, const Location& l)
{
    auto func = _currentFunction();
    auto body = std::make_shared<ModuleBuilder::Body>();
    auto location = (l != Location::None ? l : _currentFunction()->location);

    body->stmt = std::make_shared<statement::Block>(scope(), nullptr, location);
    body->scope = body->stmt->scope();
    func->bodies.push_back(body);

    if ( ! func->function->body() )
        func->function->setBody(body->stmt);

    if ( ! no_builder )
        return pushBuilder("", l);
    else
        return nullptr;
}

shared_ptr<hilti::expression::Block> ModuleBuilder::popBody()
{
    auto func = _currentFunction();
    auto body = func->bodies.back();
    func->bodies.pop_back();
    return std::make_shared<hilti::expression::Block>(body->stmt, body->stmt->location());
}

shared_ptr<BlockBuilder> ModuleBuilder::newBuilder(shared_ptr<ID> id, const Location& l)
{
    auto block = std::make_shared<statement::Block>(nullptr, id, l);
    return shared_ptr<BlockBuilder>(new BlockBuilder(block, nullptr, this));
}

shared_ptr<BlockBuilder> ModuleBuilder::newBuilder(const std::string& id, const Location& l)
{
    return newBuilder(std::make_shared<ID>(id, l), l);
}

shared_ptr<BlockBuilder> ModuleBuilder::pushBuilder(shared_ptr<BlockBuilder> builder)
{
    auto block = builder->block()->block();
    auto body = _currentBody();

    body->stmt->addStatement(block);
    block->scope()->setParent(body->stmt->scope());

    body->builders.push_back(builder);
    builder->setBody(body->stmt);
    return builder;
}

shared_ptr<BlockBuilder> ModuleBuilder::pushBuilder(shared_ptr<ID> id, const Location& l)
{
    auto location = (l != Location::None ? l : _currentBody()->stmt->location());
    return pushBuilder(newBuilder(id, l));
}

shared_ptr<BlockBuilder> ModuleBuilder::pushBuilder(const std::string& id, const Location& l)
{
    return pushBuilder(std::make_shared<ID>(id, l), l);
}

shared_ptr<BlockBuilder> ModuleBuilder::popBuilder(shared_ptr<BlockBuilder> builder)
{
    auto body = _currentBody();
    auto i = std::find(body->builders.begin(), body->builders.end(), builder);

    if ( i == body->builders.end() )
        internalError("unbalanced builder push/pop");

    body->builders.erase(i, body->builders.end());
    return builder;
}

shared_ptr<BlockBuilder> ModuleBuilder::popBuilder()
{
    auto body = _currentBody();
    auto builder = body->builders.back();
    body->builders.pop_back();
    return builder;
}

shared_ptr<hilti::Module> ModuleBuilder::module() const
{
    return _module;
}

shared_ptr<hilti::Function> ModuleBuilder::function() const
{
    return _currentFunction()->function;
}

shared_ptr<hilti::Scope> ModuleBuilder::scope() const
{
    // Return the scope from the most recent function which has a body.

    for ( auto f = _functions.rbegin(); f != _functions.rend(); f++ ) {
        if ( (*f)->bodies.size() )
            return (*f)->bodies.back()->stmt->scope();
    }

    assert(false); // Can't get here.
    return nullptr;
}

shared_ptr<BlockBuilder> ModuleBuilder::builder() const
{
    return _currentBody()->builders.back();
}

shared_ptr<hilti::expression::Block> ModuleBuilder::block() const
{
    return builder()->block();
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addGlobal(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    id = uniqueID(id, nullptr, force_unique, true);
    auto var = std::make_shared<variable::Global>(id, type, init, l);
    auto decl = std::make_shared<declaration::Variable>(id, var, l);
    _module->body()->addDeclaration(decl);
    return std::make_shared<hilti::expression::Variable>(var, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addGlobal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addGlobal(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::expression::Constant> ModuleBuilder::addConstant(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    auto const_ = ast::as<hilti::expression::Constant>(init);

    if ( ! const_ ) {
        error("constant initialized with non-constant expression", init);
        return nullptr;
    }

    if ( ! const_->canCoerceTo(type) ) {
        error("constant initialization does not match type", init);
        return nullptr;
    }

    const_ = ast::as<hilti::expression::Constant>(init->coerceTo(type));
    assert(const_);

    id = uniqueID(id, nullptr, force_unique, true);
    // FIXME: Shoudl we create a declaration::Constant instead of inserting
    // into the scope here directly?
    auto decl = std::make_shared<declaration::Constant>(id, const_->constant(), l);
    _module->body()->addDeclaration(decl);
    return const_;
}

shared_ptr<hilti::expression::Constant> ModuleBuilder::addConstant(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addConstant(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::expression::Type> ModuleBuilder::addType(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool force_unique, const Location& l)
{
    id = uniqueID(id, nullptr, force_unique, true);
    auto decl = std::make_shared<declaration::Type>(id, type, l);
    _module->body()->addDeclaration(decl);
    return std::make_shared<hilti::expression::Type>(type, l);
}

shared_ptr<hilti::expression::Type> ModuleBuilder::addType(const std::string& id, shared_ptr<Type> type, bool force_unique, const Location& l)
{
    return addType(std::make_shared<ID>(id, l), type, force_unique, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addLocal(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    id = uniqueID(id, scope(), force_unique, false);
    auto var = std::make_shared<variable::Local>(id, type, init, l);
    auto decl = std::make_shared<declaration::Variable>(id, var, l);
    _currentBody()->stmt->addDeclaration(decl);
    return std::make_shared<hilti::expression::Variable>(var, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addLocal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addLocal(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addTmp(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool reuse, const Location& l)
{
    auto nid = std::make_shared<ID>("__" + id->name(), id->location());

    if ( ! reuse )
        nid = uniqueID(nid, scope(), true, false);

    else {
        auto var = scope()->lookup(id, false);

        if ( var ) {
            auto decl = ast::as<declaration::Variable>(var);
            if ( ! (decl && decl->variable()->type()->equal(type)) ) {
                error(::util::fmt("ModuleBuilder::addTmp: ID %s already exists but is of different type", id->name().c_str()), id);
                return nullptr;
            }

            return std::make_shared<hilti::expression::Variable>(decl->variable(), l);
        }
    }

    auto var = std::make_shared<variable::Local>(id, type, init, l);
    auto decl = std::make_shared<declaration::Variable>(id, var, l);
    _currentBody()->stmt->addDeclaration(decl);
    return std::make_shared<hilti::expression::Variable>(var, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addTmp(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool reuse, const Location& l)
{
    return addTmp(std::make_shared<ID>(id, l), type, init, reuse, l);
}


void ModuleBuilder::importModule(shared_ptr<ID> id)
{
    _module->import(id);
}

void ModuleBuilder::importModule(const std::string& id)
{
    return importModule(std::make_shared<ID>(id));
}

}
}

