
#include "builder/module.h"
#include "hilti-intern.h"

namespace hilti {
namespace builder {

ModuleBuilder::ModuleBuilder(shared_ptr<CompilerContext> ctx, shared_ptr<ID> id, const std::string& path, const Location& l)
{
    Init(ctx, id, path, l);
}

ModuleBuilder::ModuleBuilder(shared_ptr<CompilerContext> ctx, const std::string& id, const std::string& path, const Location& l)
{
    Init(ctx, std::make_shared<ID>(id, l), path, l);
}

void ModuleBuilder::Init(shared_ptr<CompilerContext> ctx, shared_ptr<ID> id, const std::string& path, const Location& l)
{
    _module = builder::module::create(ctx, id, path, l);

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
    body->stmt->scope()->setID(_module->id());
}

ModuleBuilder::~ModuleBuilder()
{
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

void ModuleBuilder::exportID(const std::string& name)
{
    _module->exportID(std::make_shared<ID>(name));
}

shared_ptr<ID> ModuleBuilder::_normalizeID(shared_ptr<ID> id) const
{
    if ( id->name().find("-") )
        id = builder::id::node(util::strreplace(id->name(), "-", "_"), id->location());

    return id;
}

std::pair<shared_ptr<ID>, shared_ptr<Declaration>> ModuleBuilder::_uniqueDecl(shared_ptr<ID> id, shared_ptr<Type> type, const std::string& kind, declaration_map* decls, _DeclStyle style, bool global)
{
    id = _normalizeID(id);

    int i = 1;
    shared_ptr<ID> uid = id;

    while ( true ) {
        auto d = decls->find(uid->name());

        if ( d == decls->end() )
            return std::make_pair(uid, nullptr);

        if ( style == REUSE &&
             (*d).second.kind == kind &&
             (! (*d).second.type || (*d).second.type->equal(type)) )
            return std::make_pair(uid, (*d).second.declaration);

        if ( style == CHECK_UNIQUE )
            fatalError(::util::fmt("ModuleBuilder: ID %s already defined", uid->name()));

        std::string s = ::util::fmt("%s_%d", id->name().c_str(), ++i);
        uid = std::make_shared<ID>(s, uid->location());
    }

    assert(false); // can't be reached.
}

void ModuleBuilder::_addDecl(shared_ptr<ID> id, shared_ptr<Type> type, const std::string& kind, declaration_map* decls, shared_ptr<Declaration> decl)
{
    DeclarationMapValue dv;
    dv.kind = kind;
    dv.type = type;
    dv.declaration = decl;
    decls->insert(std::make_pair(id->name(), dv));
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::pushFunction(shared_ptr<hilti::Function> function, bool no_body)
{
    auto func = std::make_shared<ModuleBuilder::Function>();
    func->function = function;
    func->location = function->location();
    _functions.push_back(func);

    shared_ptr<declaration::Function> decl = nullptr;

    auto hook = ast::as<hilti::Hook>(function);

    if ( hook )
        decl = std::make_shared<declaration::Hook>(hook, function->location());
    else
        decl = std::make_shared<declaration::Function>(function, function->location());

    _module->body()->addDeclaration(decl);

    if ( ! no_body )
        pushBody();

    return decl;
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::declareFunction(shared_ptr<hilti::Function> func)
{
    auto decl = std::make_shared<declaration::Function>(func, func->location());
    _module->body()->addDeclaration(decl);
    return decl;
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::pushFunction(shared_ptr<ID> id,
                                                              shared_ptr<hilti::function::Result> result,
                                                              const hilti::function::parameter_list& params,
                                                              hilti::type::function::CallingConvention cc,
                                                              shared_ptr<Type> scope,
                                                              bool no_body,
                                                              const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Result>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Function>(result, params, cc, l);
    auto func = std::make_shared<hilti::Function>(id, ftype, _module, scope, nullptr, l);
    return pushFunction(func, no_body);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::pushFunction(const std::string& id,
                                                              shared_ptr<hilti::function::Result> result,
                                                              const hilti::function::parameter_list& params,
                                                              hilti::type::function::CallingConvention cc,
                                                              shared_ptr<Type> scope,
                                                              bool no_body,
                                                              const Location& l)
{
    return pushFunction(std::make_shared<ID>(id, l), result, params, cc, scope, no_body, l);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::declareHook(shared_ptr<hilti::Hook> hook)
{
    auto decl = std::make_shared<declaration::Hook>(hook, hook->location());
    _module->body()->addDeclaration(decl);
    return decl;
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::declareFunction(shared_ptr<ID> id,
                                                                       shared_ptr<hilti::function::Result> result,
                                                                       const hilti::function::parameter_list& params,
                                                                       hilti::type::function::CallingConvention cc,
                                                                       const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Result>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Function>(result, params, cc, l);
    auto func = std::make_shared<hilti::Function>(id, ftype, _module, nullptr, nullptr, l);
    return declareFunction(func);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::declareFunction(const std::string& id,
                                                                       shared_ptr<hilti::function::Result> result,
                                                                       const hilti::function::parameter_list& params,
                                                                       hilti::type::function::CallingConvention cc,
                                                                       const Location& l)
{
    return declareFunction(std::make_shared<ID>(id, l), result, params, cc, l);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::declareHook(shared_ptr<ID> id,
                                                                   shared_ptr<hilti::function::Result> result,
                                                                   const hilti::function::parameter_list& params,
                                                                   const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Result>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    auto func = std::make_shared<hilti::Hook>(id, ftype, _module, nullptr, hilti::hook::attribute_list(), nullptr, l);
    return declareHook(func);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::declareHook(const std::string& id,
                                                                   shared_ptr<hilti::function::Result> result,
                                                                   const hilti::function::parameter_list& params,
                                                                   const Location& l)
{
    return declareHook(std::make_shared<hilti::ID>(id, l), result, params, l);
}


shared_ptr<hilti::declaration::Function> ModuleBuilder::pushHook(shared_ptr<ID> id,
                                                          shared_ptr<hilti::function::Result> result,
                                                          const hilti::function::parameter_list& params,
                                                          shared_ptr<Type> scope,
                                                          const hilti::hook::attribute_list& attrs,
                                                          bool no_body,
                                                          const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Result>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    auto func =  std::make_shared<hilti::Hook>(id, ftype, _module, scope, attrs, nullptr, l);
    return pushFunction(func, no_body);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::pushHook(const std::string& id,
                                                          shared_ptr<hilti::function::Result> result,
                                                          const hilti::function::parameter_list& params,
                                                          shared_ptr<Type> scope,
                                                          const hilti::hook::attribute_list& attrs,
                                                          bool no_body,
                                                          const Location& l)
{
    return pushHook(std::make_shared<ID>(id, l), result, params, scope, attrs, no_body, l);

}

shared_ptr<hilti::declaration::Function> ModuleBuilder::pushHook(shared_ptr<ID> id,
                                                          shared_ptr<hilti::function::Result> result,
                                                          const hilti::function::parameter_list& params,
                                                          shared_ptr<Type> scope,
                                                          int64_t priority, int64_t group,
                                                          bool no_body,
                                                          const Location& l)
{
    if ( ! result )
        result = std::make_shared<hilti::function::Result>(builder::void_::type(), true);

    auto ftype = std::make_shared<hilti::type::Hook>(result, params, l);
    auto func = std::make_shared<hilti::Hook>(id, ftype, _module, scope, priority, group, nullptr, l);
    return pushFunction(func, no_body);
}

shared_ptr<hilti::declaration::Function> ModuleBuilder::pushHook(const std::string& id,
                                                          shared_ptr<hilti::function::Result> result,
                                                          const hilti::function::parameter_list& params,
                                                          shared_ptr<Type> scope,
                                                          int64_t priority, int64_t group,
                                                          bool no_body,
                                                          const Location& l)
{
    return pushHook(std::make_shared<ID>(id, l), result, params, scope, priority, group, no_body, l);
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
    if ( id ) {
        if ( id->name().size() && ! util::startsWith(id->name(), "@") )
            id = builder::id::node(util::fmt("@%s", id->name()), id->location());

        id = _normalizeID(id);
    }

    if ( id ) {
        shared_ptr<ID> nid = id;
        int cnt = 1;
        while ( _currentFunction()->labels.find(nid->name()) != _currentFunction()->labels.end() ) {
            nid = builder::id::node(util::fmt("%s_%d", id->name(), ++cnt), id->location());
        }

        id = nid;

        _currentFunction()->labels.insert(id->name());
    }

    auto block = std::make_shared<statement::Block>(nullptr, id, l);
    return shared_ptr<BlockBuilder>(new BlockBuilder(block, nullptr, this));
}

shared_ptr<BlockBuilder> ModuleBuilder::newBuilder(const std::string& id, const Location& l)
{
    return newBuilder(std::make_shared<ID>(id, l), l);
}

shared_ptr<BlockBuilder> ModuleBuilder::pushBuilder(shared_ptr<BlockBuilder> builder)
{
    auto block = builder->statement();
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

shared_ptr<BlockBuilder> ModuleBuilder::pushModuleInit()
{
    if ( ! _init ) {
        auto decl = pushFunction("__init_module", nullptr, function::parameter_list());
        decl->function()->setInitFunction();
        _init = _currentFunction();
    }

    else
        _functions.push_back(_init);

    auto body = _currentBody();
    return body->builders.back();
}

void ModuleBuilder::popModuleInit()
{
    popFunction();
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

shared_ptr<hilti::Module> ModuleBuilder::finalize(bool verify)
{
    _module->compilerContext()->addModule(_module);

    if ( ! _module->compilerContext()->finalize(verify) )
        return nullptr;

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

shared_ptr<hilti::Expression> ModuleBuilder::block() const
{
    return builder()->block();
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addGlobal(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    auto t = _uniqueDecl(id, type, "global", &_globals, (force_unique ? MAKE_UNIQUE : CHECK_UNIQUE), true);
    id = t.first;
    auto decl = t.second ? ast::checkedCast<declaration::Variable>(t.second) : nullptr;

    if ( ! decl ) {
        auto var = std::make_shared<variable::Global>(id, type, init, l);
        decl = std::make_shared<declaration::Variable>(id, var, l);
        _module->body()->addDeclaration(decl);
        _addDecl(id, type, "global", &_globals, decl);
    }

    return std::make_shared<hilti::expression::Variable>(decl->variable(), l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addGlobal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addGlobal(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::Expression> ModuleBuilder::addConstant(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    if ( ! init->isConstant() ) {
        error("constant initialized with non-constant expression", init);
        return nullptr;
    }

    if ( ! init->canCoerceTo(type) ) {
        error("constant initialization does not match type", init);
        return nullptr;
    }

    auto const_ = init->coerceTo(type);
    auto t = _uniqueDecl(id, type, "const", &_globals, (force_unique ? MAKE_UNIQUE : CHECK_UNIQUE), true);
    id = t.first;
    auto decl = t.second ? ast::checkedCast<declaration::Constant>(t.second) : nullptr;

    if ( ! decl ) {
        decl = std::make_shared<declaration::Constant>(id, const_, l);
        _module->body()->addDeclaration(decl);
        _addDecl(id, type, "const", &_globals, decl);
    }

    return std::make_shared<hilti::expression::ID>(id, l);
}

shared_ptr<hilti::Expression> ModuleBuilder::addConstant(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addConstant(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::expression::Type> ModuleBuilder::addType(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool force_unique, const Location& l)
{
    auto t = _uniqueDecl(id, type, "type", &_globals, (force_unique ? MAKE_UNIQUE : CHECK_UNIQUE), true);
    id = t.first;
    auto decl = t.second ? ast::checkedCast<declaration::Type>(t.second) : nullptr;

    if ( ! decl ) {
        decl = std::make_shared<declaration::Type>(id, type, l);
        _module->body()->addDeclaration(decl);
        _addDecl(id, type, "type", &_globals, decl);
    }

    return std::make_shared<hilti::expression::Type>(decl->type(), l);
}

shared_ptr<hilti::expression::Type> ModuleBuilder::addType(const std::string& id, shared_ptr<Type> type, bool force_unique, const Location& l)
{
    return addType(std::make_shared<ID>(id, l), type, force_unique, l);
}

shared_ptr<hilti::expression::Type> ModuleBuilder::addContext(shared_ptr<Type> type, const Location& l)
{
    auto id = std::make_shared<hilti::ID>("Context", l);
    return addType(id, type, false, l);
}



shared_ptr<hilti::expression::Variable> ModuleBuilder::addLocal(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return _addLocal(_currentBody()->stmt, id, type, init, force_unique, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::_addLocal(shared_ptr<statement::Block> stmt, shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    auto t = _uniqueDecl(id, type, "local", &_currentFunction()->locals, (force_unique ? MAKE_UNIQUE : CHECK_UNIQUE), false);
    id = t.first;
    auto decl = t.second ? ast::checkedCast<declaration::Variable>(t.second) : nullptr;

    if ( ! decl ) {
        auto var = std::make_shared<variable::Local>(id, type, init, l);
        decl = std::make_shared<declaration::Variable>(id, var, l);
        stmt->addDeclaration(decl);
        _addDecl(id, type, "local", &_currentFunction()->locals, decl);
    }

    return std::make_shared<hilti::expression::Variable>(decl->variable(), l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addLocal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique, const Location& l)
{
    return addLocal(std::make_shared<ID>(id, l), type, init, force_unique, l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addTmp(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool reuse, const Location& l)
{
    auto local = (_currentFunction()->function != nullptr);

    id = std::make_shared<ID>("__tmp_" + id->name(), id->location());

    auto t = _uniqueDecl(id, type, "tmp", local ? &_currentFunction()->locals : &_globals, (reuse ? REUSE : MAKE_UNIQUE), false);
    id = t.first;
    auto decl = t.second ? ast::checkedCast<declaration::Variable>(t.second) : nullptr;

    if ( ! decl ) {
        shared_ptr<Variable> var = nullptr;

        if ( local )
            var = std::make_shared<variable::Local>(id, type, init, l);
        else
            var = std::make_shared<variable::Global>(id, type, init, l);

        decl = std::make_shared<declaration::Variable>(id, var, l);
        _currentBody()->stmt->addDeclaration(decl);
        _addDecl(id, type, "tmp", local ? &_currentFunction()->locals : &_globals, decl);
    }

    return std::make_shared<hilti::expression::Variable>(decl->variable(), l);
}

shared_ptr<hilti::expression::Variable> ModuleBuilder::addTmp(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool reuse, const Location& l)
{
    return addTmp(std::make_shared<ID>(id, l), type, init, reuse, l);
}

void ModuleBuilder::importModule(shared_ptr<ID> id)
{
    _module->import(id);
    if ( ! _module->compilerContext()->importModule(id) )
        error("import error");
}

void ModuleBuilder::importModule(const std::string& id)
{
    return importModule(std::make_shared<ID>(id));
}

void ModuleBuilder::cacheNode(const std::string& component, const std::string& idx, shared_ptr<Node> node)
{
    auto key = component + "$" + idx;
    _node_cache[key] = node;
}

shared_ptr<Node> ModuleBuilder::lookupNode(const std::string& component, const std::string& idx)
{
    auto key = component + "$" + idx;
    auto i = _node_cache.find(key);

    return i != _node_cache.end() ? i->second : nullptr;
}

shared_ptr<BlockBuilder> ModuleBuilder::cacheBlockBuilder(const std::string& tag, cache_builder_callback cb)
{
    auto b = _currentFunction()->builders.find(tag);

    if ( b != _currentFunction()->builders.end() )
        return b->second;

    auto builder = pushBuilder(tag);
    cb();
    popBuilder(builder);

    _currentFunction()->builders.insert(std::make_pair(tag, builder));
    return builder;
}

}
}

