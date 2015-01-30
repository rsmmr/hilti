
#include <hilti/hilti.h>

#include "codegen.h"
#include "../context.h"
#include "../options.h"

#include "code-builder.h"
#include "parser-builder.h"
#include "type-builder.h"
#include "synchronizer.h"
#include "composer.h"
#include "../context.h"
#include "../attribute.h"

#include "../module.h"
#include "../statement.h"
#include "../expression.h"
#include "../grammar.h"
#include "../scope.h"

using namespace binpac;

CodeGen::CodeGen(CompilerContext* ctx)
{
    _ctx = ctx;
}

CodeGen::~CodeGen()
{
}

binpac::CompilerContext* CodeGen::context() const
{
    return _ctx;
}

const binpac::Options& CodeGen::options() const
{
    return _ctx->options();
}

shared_ptr<hilti::Module> CodeGen::compile(shared_ptr<Module> module)
{
    _compiling = true;
    _state = NONE;
    _imported_types.clear();

    _code_builder = unique_ptr<codegen::CodeBuilder>(new codegen::CodeBuilder(this));
    _parser_builder = unique_ptr<codegen::ParserBuilder>(new codegen::ParserBuilder(this));
    _type_builder = unique_ptr<codegen::TypeBuilder>(new codegen::TypeBuilder(this));
    _synchronizer = unique_ptr<codegen::Synchronizer>(new codegen::Synchronizer(this));
    _composer = unique_ptr<codegen::Composer>(new codegen::Composer(this));
    _coercer = unique_ptr<Coercer>(new Coercer());

    _parser_builder->forwardLoggingTo(this);

    try {
        hilti::path_list paths = module->context()->options().libdirs_pac2;
        auto id = hilti::builder::id::node(module->id()->name(), module->id()->location());
        auto ctx = module->context()->hiltiContext();
        _mbuilder = std::make_shared<hilti::builder::ModuleBuilder>(ctx, id, module->path(), module->location());
        _mbuilder->importModule("Hilti");
        _mbuilder->importModule("BinPACHilti");

        if ( util::strtolower(module->id()->name()) != "binpac" )
            _mbuilder->importModule("BinPAC");

        auto name = ::util::fmt("__entry");
        builder()->addInstruction(::hilti::instruction::flow::CallVoid, hilti::builder::id::create(name), hilti::builder::tuple::create({}));
        auto entry = _mbuilder->pushFunction(name);

        _state = INIT;
        _module = module;
        _code_builder->processOne(module);
        _state = NONE;

        _mbuilder->popFunction();

        // Add types that need to be imported from other modules. Note that
        // we may add further types to the list as we proceed, that's what
        // the outer loop is for.

        std::set<string> done;

        while ( _imported_types.size() != done.size() ) {
            for ( auto t : _imported_types ) {
                if ( done.find(t.first) != done.end() )
                    continue;

                auto id = std::make_shared<ID>(t.first);
                auto type = t.second;
                auto unit = ast::tryCast<type::Unit>(type);
                auto hlttype = unit ? hiltiTypeParseObject(unit) : hiltiType(type);

                hiltiAddType(id, hlttype, type);

                done.insert(t.first);
            }
        }

#if 0
        _mbuilder->module()->compilerContext()->dump(_mbuilder->module(), std::cerr);
        _mbuilder->module()->compilerContext()->print(_mbuilder->module(), std::cerr);
#endif
        _module = nullptr;

        if ( errors() )
            return nullptr;

        auto m = _mbuilder->finalize();

        if ( ! m )
            return options().verify ? nullptr : _mbuilder->module();

        return m;
    }

    catch ( const ast::FatalLoggerError& err ) {
        // Message has already been printed.
        return nullptr;
    }
}

shared_ptr<binpac::Module> CodeGen::module() const
{
    return _module;
}

shared_ptr<hilti::builder::ModuleBuilder> CodeGen::moduleBuilder() const
{
    assert(_compiling);

    return _mbuilder;
}

shared_ptr<hilti::builder::BlockBuilder> CodeGen::builder() const
{
    assert(_compiling);

    return _mbuilder->builder();
}

shared_ptr<binpac::type::Unit> CodeGen::unit() const
{
    auto u = _parser_builder->unit();
    return u;
}

shared_ptr<hilti::Expression> CodeGen::hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to)
{
    assert(_compiling);
    return _code_builder->hiltiExpression(expr, coerce_to);
}

void CodeGen::hiltiStatement(shared_ptr<Statement> stmt)
{
    assert(_compiling);
    _code_builder->hiltiStatement(stmt);
}

shared_ptr<hilti::Type> CodeGen::hiltiType(shared_ptr<Type> type, id_list* deps)
{
    assert(_compiling);
    return _type_builder->hiltiType(type, deps);
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeInteger(int width, bool signed_)
{
    return hiltiType(std::make_shared<type::Integer>(width, signed_));
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeOptional(shared_ptr<Type> t)
{
    return t ? hiltiType(std::make_shared<type::Optional>(t))
             : hiltiType(std::make_shared<type::Optional>());
}

shared_ptr<hilti::Expression> CodeGen::hiltiConstantInteger(int value, bool signed_)
{
    auto c = ::hilti::builder::integer::create(value);

    // Make sure we get out attributes.
    auto attrs = hiltiTypeInteger(64, signed_)->attributes();
    c->type()->setAttributes(attrs);
    return c;
}

shared_ptr<hilti::Expression> CodeGen::hiltiConstantOptional(shared_ptr<Expression> value)
{
    std::shared_ptr<::hilti::expression::Constant> c;

    if ( value )
        c = hilti::builder::union_::create(hiltiExpression(value));
    else
        c = hilti::builder::union_::create();

    // Make sure we get our attributes.
    auto attrs = hiltiTypeOptional(value ? value->type() : nullptr)->attributes();
    c->constant()->setAttributes(attrs);
    return c;
}

shared_ptr<hilti::ID> CodeGen::hiltiTypeID(shared_ptr<Type> type)
{
    assert(_compiling);
    return _type_builder->hiltiTypeID(type);
}

shared_ptr<hilti::Expression> CodeGen::hiltiDefault(shared_ptr<Type> type, bool null_on_default, bool can_be_unset)
{
    assert(_compiling);
    return _type_builder->hiltiDefault(type, null_on_default, can_be_unset);
}

shared_ptr<hilti::Expression> CodeGen::hiltiCoerce(shared_ptr<hilti::Expression> hexpr, shared_ptr<Type> src, shared_ptr<Type> dst)
{
    assert(_compiling);

    auto expr = std::make_shared<expression::CodeGen>(src, hexpr);
    auto cexpr = _coercer->coerceTo(expr, dst);

    if ( ! cexpr )
        internalError(util::fmt("coercion failed in CodeGen::hiltiCoerce(): cannot coerce HILTI expression of BinPAC++ type %s to type %s (%s / %s)",
                               src->render().c_str(),
                               dst->render().c_str(),
                               hexpr->render(), string(hexpr->location())));

    return hiltiExpression(cexpr);
}

shared_ptr<hilti::ID> CodeGen::hiltiID(shared_ptr<ID> id, bool qualify)
{
    auto uid = hilti::builder::id::node(id->pathAsString(), id->location());

    if ( ! qualify )
        return  uid;

    auto mod = id->firstParent<Module>();
    assert(mod);

    string name;

    if ( ! uid->isScoped() ) {
        if ( mod->id()->name() != module()->id()->name() )
            name = util::fmt("%s::%s", mod->id()->name(), uid->name());
        else
            name = uid->name();
    }

    else
        name = uid->pathAsString(hiltiID(module()->id()));

    return hilti::builder::id::node(name, id->location());
}

shared_ptr<hilti::Expression> CodeGen::hiltiParseFunction(shared_ptr<type::Unit> u)
{
    auto grammar = u->grammar();
    assert(grammar);

    auto func = ast::tryCast<hilti::Expression>(_mbuilder->lookupNode("parse-func", grammar->name()));

    if ( func )
        return func;

    _state = PARSING;
    func = _parser_builder->hiltiCreateParseFunction(u);
    _state = NONE;

    _mbuilder->cacheNode("parse-func", grammar->name(), func);
    return func;
}

shared_ptr<hilti::Expression> CodeGen::hiltiComposeFunction(shared_ptr<type::Unit> u)
{
    auto grammar = u->grammar();
    assert(grammar);

    auto func = ast::tryCast<hilti::Expression>(_mbuilder->lookupNode("compose-func", grammar->name()));

    if ( func )
        return func;

    _state = COMPOSING;
    func = _composer->hiltiCreateComposeFunction(u);
    _state = NONE;

    _mbuilder->cacheNode("compose-func", grammar->name(), func);
    return func;
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeParseObject(shared_ptr<type::Unit> unit)
{
    auto t = ast::tryCast<hilti::Type>(_mbuilder->lookupNode("parse-obj", unit->id()->name()));

    if ( t )
        return t;

    _state = PARSING;
    t = _parser_builder->hiltiTypeParseObject(unit);
    _state = NONE;

    _mbuilder->cacheNode("parse-obj", unit->id()->name(), t);
    return t;
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeParseObjectRef(shared_ptr<type::Unit> u)
{
    return hilti::builder::reference::type(hiltiTypeParseObject(u));
}

shared_ptr<hilti::Expression> CodeGen::hiltiAddParseObjectTypeInfo(shared_ptr<Type> unit)
{
    auto e = ast::tryCast<hilti::Expression>(_mbuilder->lookupNode("parse-obj-typeinfo", unit->id()->name()));

    if ( e )
        return e;

    e = _type_builder->hiltiAddParseObjectTypeInfo(unit);

    _mbuilder->cacheNode("parse-obj-typeinfo", unit->id()->name(), e);
    return e;
}

void CodeGen::hiltiAddType(shared_ptr<ID> id, shared_ptr<hilti::Type> htype, shared_ptr<Type> btype)
{
    auto hid = hiltiID(id);

    // If the ID is already declared, we assume it's equivalent.
    if ( moduleBuilder()->declared(hid) )
        return;

    moduleBuilder()->addType(hid, htype, false, htype->location());

    if ( auto unit = ast::tryCast<type::Unit>(btype) ) {
        auto hostapp_type = hiltiAddParseObjectTypeInfo(unit);
        auto i = ::hilti::builder::integer::create(BINPAC_TYPE_UNIT);
        ::hilti::builder::tuple::element_list elems = { i, hostapp_type };
        auto t = ::hilti::builder::tuple::create(elems);
        htype->attributes().add(::hilti::attribute::HOSTAPP_TYPE, t);
    }
}

void CodeGen::hiltiExportParser(shared_ptr<type::Unit> unit)
{
    _state = PARSING;
    _hiltiCreateParserInitFunction(unit);
    _state = NONE;
}

void CodeGen::hiltiUnitHooks(shared_ptr<type::Unit> unit)
{
    for ( auto i : unit->flattenedItems() ) {
        if ( ! i->type() )
            continue;

        for ( auto h : i->hooks() ) {
            auto dd = h->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;

            if ( h->composeHook() )
                _hiltiDefineHook(hookForItem(unit, i->sharedPtr<type::unit::Item>(), h->foreach(), true, true),
                                 i, h->foreach(), h->debug(), unit, h->body(), dd, h->priority(), true);

            if ( h->parseHook() )
                _hiltiDefineHook(hookForItem(unit, i->sharedPtr<type::unit::Item>(), h->foreach(), true, false),
                                 i, h->foreach(), h->debug(), unit, h->body(), dd, h->priority(), false);
        }
    }

    for ( auto g : unit->globalHooks() ) {
        for ( auto h : g->hooks() ) {
            if ( util::startsWith(g->id()->local(), "%") ) {
                if ( h->composeHook() )
                    _hiltiDefineHook(hookForUnit(unit, g->id()->local(), true), nullptr, false, h->debug(), unit, h->body(), nullptr, h->priority(), true);

                if ( h->parseHook() )
                    _hiltiDefineHook(hookForUnit(unit, g->id()->local(), false), nullptr, false, h->debug(), unit, h->body(), nullptr, h->priority(), false);
            }

            else {
                auto i = unit->item(g->id());
                assert(i && i->type());
                auto dd = h->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;

                if ( h->composeHook() )
                    _hiltiDefineHook(hookForItem(unit, i, h->foreach(), false, true), i, h->foreach(), h->debug(), unit, h->body(), dd, h->priority(), true);

                if ( h->parseHook() )
                    _hiltiDefineHook(hookForItem(unit, i, h->foreach(), false, false), i, h->foreach(), h->debug(), unit, h->body(), dd, h->priority(), false);
            }
        }
    }
}

void CodeGen::hiltiRunFieldHooks(shared_ptr<binpac::type::Unit> unit, shared_ptr<type::unit::Item> item, shared_ptr<hilti::Expression> self, bool compose, shared_ptr<hilti::Expression> cookie)
{
    hiltiRunHook(unit, self, hookForItem(unit, item, false, true, compose), item, false, nullptr, false, cookie);
    hiltiRunHook(unit, self, hookForItem(unit, item, false, false, compose), item, false, nullptr, false, cookie);
}

void CodeGen::hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook)
{
    auto unit = hook->unit();
    assert(unit);

    if ( util::startsWith(id->local(), "%") ) {
        if ( hook->composeHook() )
            _hiltiDefineHook(hookForUnit(unit, id->local(), true), nullptr, false, hook->debug(), unit, hook->body(), nullptr, hook->priority(), true);

        if ( hook->parseHook() )
            _hiltiDefineHook(hookForUnit(unit, id->local(), false), nullptr, false, hook->debug(), unit, hook->body(), nullptr, hook->priority(), false);
    }

    else {
        auto i = unit->item(id);
        assert(i && i->type());
        auto dd = hook->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;

        if ( hook->composeHook() )
            _hiltiDefineHook(hookForItem(unit, i, hook->foreach(), false, true), i, hook->foreach(), hook->debug(), unit, hook->body(), dd, hook->priority(), true);

        if ( hook->parseHook() )
            _hiltiDefineHook(hookForItem(unit, i, hook->foreach(), false, false), i, hook->foreach(), hook->debug(), unit, hook->body(), dd, hook->priority(), false);
    }
}

shared_ptr<hilti::Expression> CodeGen::hiltiRunHook(shared_ptr<binpac::type::Unit> unit, shared_ptr<hilti::Expression> self, shared_ptr<ID> id, shared_ptr<type::unit::Item> item, bool foreach, shared_ptr<hilti::Expression> dollardollar, bool compose, shared_ptr<hilti::Expression> cookie)
{
    if ( item && ! item->hooksEnabled() ) {
        if ( foreach )
            std::make_shared<expression::Constant>(std::make_shared<constant::Bool>(false)); // ???
        else
            return nullptr;
    }

    if ( options().debug > 0 ) {
        auto msg = util::fmt("- - triggering hook %s", id->pathAsString());
        builder()->addDebugMsg("binpac-verbose", msg);
    }

    auto name = id->pathAsString();

    // TODO: Don't need "local" anymore I believe.

    auto unit_module = unit->firstParent<Module>();
    assert(unit_module);

    if ( unit_module->id()->name() != module()->id()->name() )
        name = util::fmt("%s::%s", unit_module->id()->name(), name);

    // Declare the hook if we don't have done that yet.
    if ( ! moduleBuilder()->lookupNode("hook", name) ) {
        hilti::builder::function::parameter_list p = {
            hilti::builder::function::parameter("__self", hiltiTypeParseObjectRef(unit), false, nullptr),
            hilti::builder::function::parameter("__cookie", hiltiTypeCookie(), false, nullptr)
        };

        if ( dollardollar )
            p.push_back(hilti::builder::function::parameter("__dollardollar", dollardollar->type(), false, nullptr));

        shared_ptr<hilti::Type> rtype = nullptr;

        if ( foreach )
            rtype = hilti::builder::boolean::type();
        else
            rtype = hilti::builder::void_::type();

        auto hook = moduleBuilder()->declareHook(name, hilti::builder::function::result(rtype), p);

        moduleBuilder()->cacheNode("hook", name, hook);
    }

    // Run the hook.
    hilti::builder::tuple::element_list args = { self, cookie };

    if ( dollardollar )
        args.push_back(dollardollar);

    shared_ptr<hilti::Expression> result;

    if ( ! foreach ) {
        builder()->addInstruction(hilti::instruction::hook::Run,
                                        hilti::builder::id::create(name),
                                        hilti::builder::tuple::create(args));
        result = nullptr;
    }

    else {
        auto stop = moduleBuilder()->addTmp("hook_result", hilti::builder::boolean::type(), hilti::builder::boolean::create(false));
        builder()->addInstruction(stop,
                                        hilti::instruction::hook::Run,
                                        hilti::builder::id::create(name),
                                        hilti::builder::tuple::create(args));
        result = stop;
    }

    return result;
}

shared_ptr<binpac::ID> CodeGen::hookForItem(shared_ptr<type::Unit> unit, shared_ptr<type::unit::Item> item, bool foreach, bool private_, bool compose)
{
    string fe = foreach ? "%foreach" : "";
    string pr = private_ ? util::fmt("%%intern%%%p", item.get()) : "%extern";
    string co = compose ? string("compose_") : string("_parse_");

    auto hook = util::fmt("%s_%s%s%s", unit->id()->name(), item->id()->name(), fe, pr);
    hook = util::strreplace(hook, "::", "_");
    hook = util::strreplace(hook, "%", "__0x37");
    hook = string("__hook_") + co + hook;

    return std::make_shared<binpac::ID>(hook);
}

shared_ptr<binpac::ID> CodeGen::hookForUnit(shared_ptr<type::Unit> unit, const string& name, bool compose)
{
    string co = compose ? string("compose_") : string("_parse_");

    auto hook = util::fmt("%s_%s", unit->id()->name(), name);
    hook = util::strreplace(hook, "::", "_");
    hook = util::strreplace(hook, "%", "__0x37");
    hook = string("__hook_") + co + hook;

    return std::make_shared<binpac::ID>(hook);
}

void CodeGen::_hiltiDefineHook(shared_ptr<ID> id, shared_ptr<type::unit::Item> item, bool foreach, bool debug, shared_ptr<type::Unit> unit, shared_ptr<Statement> body, shared_ptr<Type> dollardollar, int priority, bool compose)
{
    auto unit_module = unit->firstParent<Module>();
    auto current_module = module();

    if ( debug && options().debug == 0 )
        return;

    auto name = id->pathAsString();

    if ( unit_module->id()->name() != current_module->id()->name() )
        name = util::fmt("%s::%s", unit_module->id()->name(), name);

    hilti::builder::function::parameter_list p = {
        hilti::builder::function::parameter("__self", hiltiTypeParseObjectRef(unit), false, nullptr),
        hilti::builder::function::parameter("__cookie", hiltiTypeCookie(), false, nullptr)
    };

    if ( dollardollar ) {
        auto parseable = ast::type::tryTrait<type::trait::Parseable>(dollardollar)->fieldType();
        p.push_back(hilti::builder::function::parameter("__dollardollar", hiltiType(parseable), false, nullptr));
        hiltiBindDollarDollar(hilti::builder::id::create("__dollardollar"));
    }

    shared_ptr<hilti::Type> rtype = nullptr;

    if ( foreach )
        rtype = hilti::builder::boolean::type();
    else
        rtype = hilti::builder::void_::type();

    moduleBuilder()->pushHook(name, hilti::builder::function::result(rtype), p, hilti::AttributeSet(), priority, 0, false);

    shared_ptr<hilti::builder::BlockBuilder> hook = nullptr;
    shared_ptr<hilti::builder::BlockBuilder> cont = nullptr;

    if ( debug ) {
        // Add guard code to check whether debug output has been enabled at
        // run-time.
        hook = moduleBuilder()->newBuilder("debug_hook");
        cont = moduleBuilder()->newBuilder("debug_cont");
        auto dbg = builder()->addTmp("dbg", hilti::builder::boolean::type());

        builder()->addInstruction(dbg, hilti::instruction::flow::CallResult,
                                        hilti::builder::id::create("BinPACHilti::debugging_enabled"),
                                        hilti::builder::tuple::create({}));

        builder()->addInstruction(hilti::instruction::flow::IfElse, dbg, hook->block(), cont->block());

        moduleBuilder()->pushBuilder(hook);
    }

//    pushState(std::make_shared<ParserState>(unit));    TODO: NEED?

    if ( options().debug > 0 ) {
        auto msg = util::fmt("- - executing hook %s@%s", id->pathAsString(), string(body->location()));
        builder()->addDebugMsg("binpac-verbose", msg);
    }

    // TODO: We disable hooks for the item to avoid cycles when the code
    // triggers the same hook again. However, this doesn't catch assigments
    // made from functions called by a hook, but there's no way to catch
    // these statically and adding a dynamic check to every hooks seems
    // overkill. Not sure what to do ...

    if ( item )
        item->disableHooks();

    hiltiStatement(body);

    if ( item )
        item->enableHooks();

    if ( dollardollar )
        hiltiUnbindDollarDollar();

//    popState();    TODO: NEED?

    if ( hook ) {
        builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
        moduleBuilder()->popBuilder(hook);
        moduleBuilder()->pushBuilder(cont); // Leave on stack.
    }

    moduleBuilder()->popHook();
}

std::pair<bool, string> CodeGen::_hookName(const string& path)
{
    bool local = false;
    auto name = path;

    // If the module part of the ID matches the current module, remove.
    auto curmod = moduleBuilder()->module()->id()->name();

    if ( util::startsWith(name, curmod + "::") ) {
        local = true;
        name = name.substr(curmod.size() + 2, string::npos);
    }

    name = util::strreplace(name, "::", "_");
    name = util::strreplace(name, "%", "__0x37");
    name = string("__hook_") + name;

    return std::make_pair(local, name);
}

shared_ptr<hilti::declaration::Function> CodeGen::hiltiDefineFunction(shared_ptr<expression::Function> func, bool declare_only)
{
    return hiltiDefineFunction(func->function(), declare_only, func->scope());
}

shared_ptr<hilti::declaration::Function> CodeGen::hiltiDefineFunction(shared_ptr<Function> func, bool declare_only, const string& scope)
{
#if 0
    // If it's scoped and we import the module, assume the module declares
    // the function.
    if ( func->id()->isScoped() && moduleBuilder()->idImported(func->id()->path().front()))
        return nullptr;
#endif

    auto name = hiltiFunctionName(func, scope);
    auto ftype = func->type();
    auto rtype = ftype->result() ? hiltiType(ftype->result()->type()) : hilti::builder::void_::type();
    auto result = hilti::builder::function::result(rtype);

    hilti::builder::function::parameter_list params;

    for ( auto p : ftype->parameters() ) {
        shared_ptr<hilti::Expression> def = p->default_() ? hiltiExpression(p->default_()) : nullptr;
        auto hp = hilti::builder::function::parameter(hiltiID(p->id()), hiltiType(p->type()), p->constant(), def);
        params.push_back(hp);
    }

    auto cookie = hilti::builder::function::parameter("__cookie", hiltiTypeCookie(), false, nullptr);

    hilti::type::function::CallingConvention cc;

    switch ( ftype->callingConvention() ) {
     case type::function::BINPAC:
        cc = hilti::type::function::HILTI;
        params.push_back(cookie);
        break;

     case type::function::BINPAC_HILTI_C:
        cc = hilti::type::function::HILTI_C;
        params.push_back(cookie);
        break;

     case type::function::BINPAC_HILTI:
        cc = hilti::type::function::HILTI;
        params.push_back(cookie);
        break;

     case type::function::HILTI_C:
        cc = hilti::type::function::HILTI_C;
        break;

     case type::function::HILTI:
        cc = hilti::type::function::HILTI;
        break;

     case type::function::C:
        cc = hilti::type::function::C;
        break;

     default:
        internalError("unexpected calling convention in hiltiDefineFunction()");
    }

    hilti::builder::attribute_set attrs;

    // TODO: Do we always want to export these?
    if ( ! declare_only )
        moduleBuilder()->exportID(name);

    if ( func->body() && ! declare_only ) {
        auto decl = moduleBuilder()->pushFunction(name, result, params, cc, attrs, false, func->location());
        hiltiStatement(func->body());
        moduleBuilder()->popFunction();
        return decl;
    }

    else
        return moduleBuilder()->declareFunction(name, result, params, cc, attrs, func->location());
}

shared_ptr<hilti::ID> CodeGen::hiltiFunctionName(shared_ptr<binpac::Function> func, const string& scope)
{
    if ( func->hiltiFunctionID() )
        return hiltiID(func->hiltiFunctionID());

    auto id = func->id()->pathAsString();

    if ( ! func->id()->isScoped() && scope.size() )
        id = util::fmt("%s::%s", scope, id);

    if ( func->type()->callingConvention() != type::function::BINPAC )
        return hilti::builder::id::node(id, func->id()->location());

    // To make overloaded functions unique, we add a hash of the signature.
    auto type = func->type()->render();
    auto name = util::fmt("%s__%s", id, util::uitoa_n(util::hash(type), 62, 4));

    return hilti::builder::id::node(name, func->location());
}

shared_ptr<hilti::ID> CodeGen::hiltiFunctionName(shared_ptr<expression::Function> expr)
{
    return hiltiFunctionName(expr->function(), expr->scope());
}

shared_ptr<hilti::Expression> CodeGen::hiltiCall(shared_ptr<expression::Function> func, const expression_list& args, shared_ptr<hilti::Expression> cookie)
{
    auto ftype = func->function()->type();
    auto hilti_func = hiltiExpression(func);

    hilti::builder::tuple::element_list hilti_arg_list;

    auto params = ftype->parameters();
    auto p = params.begin();

    for ( auto a : args )
        hilti_arg_list.push_back(hiltiExpression(a, (*p++)->type()));

    if ( ftype->callingConvention() == type::function::BINPAC_HILTI_C ||
         ftype->callingConvention() == type::function::HILTI_C ||
         ftype->callingConvention() == type::function::HILTI ||
         ftype->callingConvention() == type::function::BINPAC_HILTI ) {
        if ( func->scope().size() )
            hiltiDefineFunction(func->function(), false, func->scope());
    }

    switch ( ftype->callingConvention() ) {
     case type::function::BINPAC:
     case type::function::BINPAC_HILTI:
     case type::function::BINPAC_HILTI_C: {
        hilti_arg_list.push_back(cookie);
        break;
     }

     case type::function::HILTI:
     case type::function::HILTI_C:
     case type::function::C:
        break;

     default:
        internalError("unexpected calling convention in expression::operator_::function::Call()");
    }

    auto hilti_args = hilti::builder::tuple::create(hilti_arg_list);

    shared_ptr<hilti::Expression> result = nullptr;

    if ( ftype->result() && ! ast::isA<type::Void>(ftype->result()->type()) ) {
        result = builder()->addTmp("result", hiltiType(ftype->result()->type()));
        builder()->addInstruction(result, hilti::instruction::flow::CallResult, hilti_func, hilti_args);
    }

    else {
        result = hilti::builder::id::create("<no return value>"); // Dummy ID; this should never be accessed ...
        builder()->addInstruction(hilti::instruction::flow::CallVoid, hilti_func, hilti_args);
    }

    return result;
}

shared_ptr<hilti::Expression> CodeGen::hiltiSelf()
{
    if ( _state == INIT )
        return nullptr;

    return hilti::builder::id::create("__self");
}

shared_ptr<hilti::Expression> CodeGen::hiltiCookie()
{
    if ( _state == INIT )
        return nullptr;

    return hilti::builder::id::create("__cookie");
}

void CodeGen::hiltiConfirm(shared_ptr<hilti::Expression> self, shared_ptr<binpac::type::Unit> unit)
{
    _parser_builder->hiltiConfirm(self, unit);
}

void CodeGen::hiltiDisable(shared_ptr<hilti::Expression> self, shared_ptr<binpac::type::Unit> unit, const string& msg)
{
    _parser_builder->hiltiDisable(self, unit, msg);
}

void CodeGen::hiltiDisable(shared_ptr<hilti::Expression> self, shared_ptr<binpac::type::Unit> unit, shared_ptr<hilti::Expression> msg)
{
    _parser_builder->hiltiDisable(self, unit, msg);
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeCookie()
{
    return hilti::builder::type::byName("BinPACHilti::UserCookie");
}

void CodeGen::hiltiBindDollarDollar(shared_ptr<hilti::Expression> val)
{
    _code_builder->hiltiBindDollarDollar(val);
}

void CodeGen::hiltiUnbindDollarDollar()
{
    _code_builder->hiltiBindDollarDollar(0);
}

shared_ptr<hilti::Expression> CodeGen::hiltiFunctionNew(shared_ptr<type::Unit> unit)
{
    _state = PARSING;
    auto e = _parser_builder->hiltiFunctionNew(unit);
    _state = NONE;
    return e;
}

shared_ptr<hilti::Expression> CodeGen::hiltiCastEnum(shared_ptr<hilti::Expression> val, shared_ptr<hilti::Type> dst)
{
    auto i = builder()->addTmp("eval", hiltiTypeInteger(64, false));
    auto e = builder()->addTmp("enum", dst);
    builder()->addInstruction(i, hilti::instruction::enum_::ToInt, val);
    builder()->addInstruction(e, hilti::instruction::enum_::FromInt, i);
    return e;
}

void CodeGen::hiltiWriteToSinks(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> seq, shared_ptr<hilti::Expression> len)
{
    _parser_builder->hiltiWriteToSinks(field, data, seq, len);
}

void CodeGen::hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> seq, shared_ptr<hilti::Expression> len)
{
    _parser_builder->hiltiWriteToSink(sink, data, seq, len);
}

void CodeGen::hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> begin, shared_ptr<hilti::Expression> end, shared_ptr<hilti::Expression> seq, shared_ptr<hilti::Expression> len)
{
    auto data = builder()->addTmp("data", hilti::builder::reference::type(hilti::builder::bytes::type()));
    builder()->addInstruction(data, hilti::instruction::bytes::Sub, begin, end);
    return hiltiWriteToSink(sink, data, seq, len);
}

shared_ptr<hilti::Expression> CodeGen::hiltiApplyAttributesToValue(shared_ptr<hilti::Expression> val, shared_ptr<AttributeSet> attrs, bool composing)
{
    auto convert = attrs->lookup("convert");

    if ( convert && ! composing ) {
        hiltiBindDollarDollar(val);
        val = hiltiExpression(convert->value());
        hiltiUnbindDollarDollar();
    }

    auto convert_back = attrs->lookup("convert_back");

    if ( convert_back && composing ) {
        hiltiBindDollarDollar(val);
        val = hiltiExpression(convert_back->value());
        hiltiUnbindDollarDollar();
    }

    return val;
}

void CodeGen::hiltiImportType(shared_ptr<ID> id, shared_ptr<Type> t)
{
    _imported_types.insert(std::make_pair(id->pathAsString(), t));
}

shared_ptr<hilti::Expression> CodeGen::hiltiByteOrder(shared_ptr<Expression> expr)
{
    auto t1 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Little"),
        hilti::builder::id::create(string("Hilti::ByteOrder::Little"))});

    auto t2 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Big"),
        hilti::builder::id::create(string("Hilti::ByteOrder::Big"))});

    auto t3 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Network"),
        hilti::builder::id::create(string("Hilti::ByteOrder::Big"))});

    auto t4 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Host"),
        hilti::builder::id::create(string("Hilti::ByteOrder::Host"))});

    auto tuple = hilti::builder::tuple::create({ t1, t2, t3, t4 });
    auto result = moduleBuilder()->addTmp("order", hilti::builder::type::byName("Hilti::ByteOrder"));
    auto op = hiltiExpression(expr);

    builder()->addInstruction(result, hilti::instruction::Misc::SelectValue, op, tuple);

    return result;
}

shared_ptr<hilti::Expression> CodeGen::hiltiCharset(shared_ptr<Expression> expr)
{
    auto t1 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::Charset::UTF8"),
        hilti::builder::id::create(string("Hilti::Charset::UTF8"))});

    auto t2 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::Charset::UTF16LE"),
        hilti::builder::id::create(string("Hilti::Charset::UTF16LE"))});

    auto t3 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::Charset::UTF16BE"),
        hilti::builder::id::create(string("Hilti::Charset::UTF16BE"))});

    auto t4 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::Charset::UTF32LE"),
        hilti::builder::id::create(string("Hilti::Charset::UTF32LE"))});

    auto t5 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::Charset::UTF32BE"),
        hilti::builder::id::create(string("Hilti::Charset::UTF32BE"))});

    auto t6 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::Charset::ASCII"),
        hilti::builder::id::create(string("Hilti::Charset::ASCII"))});

    auto tuple = hilti::builder::tuple::create({ t1, t2, t3, t4, t5, t6 });
    auto result = moduleBuilder()->addTmp("order", hilti::builder::type::byName("Hilti::Charset"));
    auto op = hiltiExpression(expr);

    builder()->addInstruction(result, hilti::instruction::Misc::SelectValue, op, tuple);

    return result;
}

shared_ptr<hilti::Expression> CodeGen::hiltiExtractsBitsFromInteger(shared_ptr<hilti::Expression> value, shared_ptr<Type> type, shared_ptr<Expression> border, shared_ptr<hilti::Expression> lower_in, shared_ptr<hilti::Expression> upper_in)
{
    auto itype = ast::checkedCast<type::Integer>(type);
    auto hitype = hiltiTypeInteger(itype->width(), false);
    auto lower = builder()->addTmp("lower", hitype);
    auto upper = builder()->addTmp("upper", hitype);
    auto tmp = builder()->addTmp("tmpbits", hitype);

    builder()->addInstruction(lower, hilti::instruction::operator_::Assign, lower_in);
    builder()->addInstruction(upper, hilti::instruction::operator_::Assign, upper_in);

    shared_ptr<hilti::Expression> result = builder()->addTmp("bits", hitype);

    // Apply bit order.

    hilti::builder::BlockBuilder::case_list cases;

    auto done = moduleBuilder()->newBuilder("bitorder-done");

    auto blsb0 = moduleBuilder()->pushBuilder("lsb0");
    // Nothing to do.
    builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    moduleBuilder()->popBuilder(blsb0);

    auto bmsb0 = moduleBuilder()->pushBuilder("msb0");

    // Invert indices.
    auto w = hilti::builder::integer::create(itype->width() - 1);
    builder()->addInstruction(tmp, hilti::instruction::integer::Sub, w, upper);
    builder()->addInstruction(upper, hilti::instruction::integer::Sub, w, lower);
    builder()->addInstruction(lower, hilti::instruction::operator_::Assign, tmp);

    builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    moduleBuilder()->popBuilder(bmsb0);

    auto bdefault = moduleBuilder()->pushBuilder("unknown");
    builder()->addInternalError("unexpected bit order");
    builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    moduleBuilder()->popBuilder(bdefault);

    auto elsb0 = hilti::builder::id::create(hiltiID(std::make_shared<ID>("BinPAC::BitOrder::LSB0")));
    auto emsb0 = hilti::builder::id::create(hiltiID(std::make_shared<ID>("BinPAC::BitOrder::MSB0")));

    cases.push_back(std::make_pair(elsb0, blsb0));
    cases.push_back(std::make_pair(emsb0, bmsb0));

    auto etype_expr = module()->body()->scope()->lookupUnique(std::make_shared<ID>("BinPAC::BitOrder"));
    if ( ! etype_expr )
        fatalError("type BinPAC::BitOrder missing");

    auto etype = ast::checkedCast<expression::Type>(etype_expr)->typeValue();
    auto eval = hiltiExpression(border, etype);

    builder()->addSwitch(eval, bdefault, cases);

    moduleBuilder()->pushBuilder(done);

    //

    builder()->addInstruction(result, hilti::instruction::integer::Mask, value, lower, upper);

    return result;
}

shared_ptr<hilti::Expression> CodeGen::hiltiInsertBitsIntoInteger(shared_ptr<hilti::Expression> value, shared_ptr<Type> type, shared_ptr<Expression> border, shared_ptr<hilti::Expression> lower_in, shared_ptr<hilti::Expression> upper_in)
{
    auto itype = ast::checkedCast<type::Integer>(type);
    auto hitype = hiltiTypeInteger(itype->width(), false);
    auto lower = builder()->addTmp("lower", hitype);
    auto upper = builder()->addTmp("upper", hitype);
    auto tmp = builder()->addTmp("tmpbits", hitype);

    builder()->addInstruction(lower, hilti::instruction::operator_::Assign, lower_in);
    builder()->addInstruction(upper, hilti::instruction::operator_::Assign, upper_in);

    /// TODO: This code here for the bitorder is identical with the
    /// corresponding part in hiltiExtractBitsFromInteger(). Should factor
    /// that out.

    // Apply bit order.

    hilti::builder::BlockBuilder::case_list cases;

    auto done = moduleBuilder()->newBuilder("bitorder-done");

    auto blsb0 = moduleBuilder()->pushBuilder("lsb0");
    // Nothing to do.
    builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    moduleBuilder()->popBuilder(blsb0);

    auto bmsb0 = moduleBuilder()->pushBuilder("msb0");

    // Invert indices.
    auto w = hilti::builder::integer::create(itype->width() - 1);
    builder()->addInstruction(tmp, hilti::instruction::integer::Sub, w, upper);
    builder()->addInstruction(upper, hilti::instruction::integer::Sub, w, lower);
    builder()->addInstruction(lower, hilti::instruction::operator_::Assign, tmp);

    builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    moduleBuilder()->popBuilder(bmsb0);

    auto bdefault = moduleBuilder()->pushBuilder("unknown");
    builder()->addInternalError("unexpected bit order");
    builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    moduleBuilder()->popBuilder(bdefault);

    auto elsb0 = hilti::builder::id::create(hiltiID(std::make_shared<ID>("BinPAC::BitOrder::LSB0")));
    auto emsb0 = hilti::builder::id::create(hiltiID(std::make_shared<ID>("BinPAC::BitOrder::MSB0")));

    cases.push_back(std::make_pair(elsb0, blsb0));
    cases.push_back(std::make_pair(emsb0, bmsb0));

    auto etype_expr = module()->body()->scope()->lookupUnique(std::make_shared<ID>("BinPAC::BitOrder"));
    if ( ! etype_expr )
        fatalError("type BinPAC::BitOrder missing");

    auto etype = ast::checkedCast<expression::Type>(etype_expr)->typeValue();
    auto eval = hiltiExpression(border, etype);

    builder()->addSwitch(eval, bdefault, cases);

    moduleBuilder()->pushBuilder(done);

    //// End of common code.

    auto result = builder()->addTmp("bits", hitype);
    builder()->addInstruction(result, hilti::instruction::integer::Shl, value, lower);

    auto rshift = builder()->addTmp("rshift", hitype);
    builder()->addInstruction(rshift, hilti::instruction::integer::Sub, hilti::builder::integer::create(itype->width()), upper);
    builder()->addInstruction(rshift, hilti::instruction::integer::Sub, rshift, hilti::builder::integer::create(1));

    auto mask = builder()->addTmp("mask", hitype);
    builder()->addInstruction(mask, hilti::instruction::integer::Shr, hilti::builder::integer::create(-1), rshift);

    builder()->addInstruction(result, hilti::instruction::integer::And, result, mask);

    return result;
}

shared_ptr<hilti::Expression> CodeGen::hiltiSynchronize(shared_ptr<Production> p, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> cur)
{
    return _synchronizer->hiltiSynchronize(p, data, cur);
}

shared_ptr<hilti::Expression> CodeGen::hiltiSynchronize(shared_ptr<type::Unit> unit, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> cur)
{
    return _synchronizer->hiltiSynchronize(unit, data, cur);
}

static binpac::type::unit::item::field::Switch* _switch(shared_ptr<binpac::type::unit::Item> item)
{
    auto f = ast::tryCast<binpac::type::unit::item::Field>(item);

    if ( ! (f && f->parent()) )
        return nullptr;

    return dynamic_cast<binpac::type::unit::item::field::Switch *>(f->parent());
}

static bool _usingStructForItems(shared_ptr<binpac::type::unit::Item> f)
{
    auto sw = _switch(f);

    if ( ! sw )
        return false;

    auto c = sw->case_(f);
    return c && (c->fields().size() > 1);
}

static shared_ptr<hilti::Type> _structType(CodeGen* cg, const string& uniq)
{
    auto n = ::util::fmt("__struct_%s", uniq);

    if ( auto t = cg->moduleBuilder()->lookupType(n) )
        return t;

    return hilti::builder::type::byName(n);
}

static shared_ptr<hilti::Expression> _structTmp(CodeGen* cg, const string& uniq)
{
    auto stype = ::hilti::builder::reference::type(_structType(cg, uniq));
    auto s = cg->moduleBuilder()->addTmp("s", stype);
    return s;
}

static std::string _structFieldNameAsString(const string& uniq)
{
    return ::util::fmt("items_%s", uniq);
}

static shared_ptr<hilti::Expression> _structFieldName(const string& uniq)
{
    return ::hilti::builder::string::create(_structFieldNameAsString(uniq));
}

static shared_ptr<hilti::Type> _unionType(CodeGen* cg, const string& uniq)
{
    auto n = ::util::fmt("__union_%s", uniq);
    return hilti::builder::type::byName(n);
}

static shared_ptr<hilti::Expression> _unionTmp(CodeGen* cg, const string& uniq)
{
    auto utype = _unionType(cg, uniq);
    auto u = cg->moduleBuilder()->addTmp("u", utype);
    return u;
}

static std::string _unionFieldNameAsString(const string& uniq)
{
    return ::util::fmt("switch_%s", uniq);
}

static shared_ptr<hilti::Expression> _unionFieldName(const string& uniq)
{
    return ::hilti::builder::string::create(_unionFieldNameAsString(uniq));
}

typedef std::function<void (void)> _callback_t;

static void _ifSet(CodeGen* cg, shared_ptr<hilti::Instruction> ins, shared_ptr<hilti::Expression> o, shared_ptr<hilti::Expression> fn, _callback_t yes_branch, _callback_t no_branch)
{
    auto has = cg->builder()->addTmp("is_set", ::hilti::builder::boolean::type());
    cg->builder()->addInstruction(has, ins, o, fn);

    auto branches = cg->builder()->addIfElse(has);
    auto yes = std::get<0>(branches);
    auto no = std::get<1>(branches);
    auto done = std::get<2>(branches);

    cg->moduleBuilder()->pushBuilder(yes);
    yes_branch();
    cg->builder()->addInstruction(::hilti::instruction::flow::Jump, done->block());
    cg->moduleBuilder()->popBuilder();

    cg->moduleBuilder()->pushBuilder(no);
    no_branch();
    cg->builder()->addInstruction(::hilti::instruction::flow::Jump, done->block());
    cg->moduleBuilder()->popBuilder();

    cg->moduleBuilder()->pushBuilder(done);
}

// struct pobj {
//     string a;
//     union _switch {
//         bool b
//         string c
//         ref<struct> d;
//     }
//     bool e;
// }

static void _getFieldInStruct(CodeGen* cg, shared_ptr<hilti::Expression> result, shared_ptr<hilti::Expression> s, shared_ptr<hilti::Expression> fn, shared_ptr<binpac::type::unit::Item> f, shared_ptr<hilti::Expression> top_level_s, bool op_is_set);
static void _getFieldInUnion(CodeGen* cg, shared_ptr<hilti::Expression> result, shared_ptr<hilti::Expression> u, shared_ptr<hilti::Expression> fn, shared_ptr<binpac::type::unit::Item> f, shared_ptr<hilti::Expression> top_level_s, bool op_is_set);
static void _presetDefaultInStruct(CodeGen* cg, shared_ptr<hilti::Expression> s, shared_ptr<hilti::Expression> fn, shared_ptr<hilti::Expression> value, shared_ptr<binpac::type::unit::Item> f, shared_ptr<hilti::Expression> top_level_s);

static void _setFieldInStruct(CodeGen* cg, shared_ptr<hilti::Expression> s, shared_ptr<hilti::Expression> fn, shared_ptr<hilti::Expression> value, shared_ptr<binpac::type::unit::Item> f);
static void _setFieldInUnion(CodeGen* cg, shared_ptr<hilti::Expression> u, shared_ptr<hilti::Expression> fn, shared_ptr<hilti::Expression> value, shared_ptr<binpac::type::unit::Item> f);

static void _computeFieldPathInStruct(CodeGen* cg, shared_ptr<hilti::Type> s, shared_ptr<binpac::type::unit::Item> f, shared_ptr<binpac::ID> fn, ::hilti::builder::tuple::element_list* path);
static void _computeFieldPathInUnion(CodeGen* cg, shared_ptr<hilti::Type> u, shared_ptr<binpac::type::unit::Item> f, shared_ptr<binpac::ID> fn, ::hilti::builder::tuple::element_list* path);


static void _getFieldInStruct(CodeGen* cg, shared_ptr<hilti::Expression> result, shared_ptr<hilti::Expression> s, shared_ptr<hilti::Expression> fn, shared_ptr<binpac::type::unit::Item> f, shared_ptr<hilti::Expression> top_level_s, bool op_is_set)
{
    shared_ptr<hilti::Instruction> ins_struct;

    if ( op_is_set )
        ins_struct = ::hilti::instruction::struct_::IsSet;
    else
        ins_struct = ::hilti::instruction::struct_::Get;

    if ( auto sw = _switch(f) ) {
        auto u = _unionTmp(cg, sw->uniqueName());
        auto ufn = _unionFieldName(sw->uniqueName());

        if ( ! (f && f->attributes()->lookup("default")) ) {
            if ( op_is_set ) {
                _ifSet(cg, ::hilti::instruction::struct_::IsSet, s, ufn,
                       [&] () { // Union field in struct is set.
                           cg->builder()->addInstruction(u, ::hilti::instruction::struct_::Get, s, ufn);
                           _getFieldInUnion(cg, result, u, fn, f, nullptr, op_is_set);
                       },

                       [&] () { // Union field in struct is not set.
                           cg->builder()->addInstruction(result, ::hilti::instruction::operator_::Assign, ::hilti::builder::boolean::create(false));
                       });
            }

            else {
                cg->builder()->addInstruction(u, ::hilti::instruction::struct_::Get, s, ufn);
                _getFieldInUnion(cg, result, u, fn, f, nullptr, op_is_set);
            }
        }

        else {
            _ifSet(cg, ::hilti::instruction::struct_::IsSet, s, ufn,
                   [&] () { // Union field in struct is set.
                       cg->builder()->addInstruction(u, ::hilti::instruction::struct_::Get, s, ufn);
                       _getFieldInUnion(cg, result, u, fn, f, s, op_is_set);
                   },

                   [&] () { // Union field in struct is not set.
                       auto dn = ::hilti::builder::string::create(::util::fmt("__default_%s", f->id()->name()));
                       cg->builder()->addInstruction(result, ins_struct, top_level_s, dn);
                   });
        }
    }

    else
        cg->builder()->addInstruction(result, ins_struct, s, fn);
}

static void _getFieldInUnion(CodeGen* cg, shared_ptr<hilti::Expression> result, shared_ptr<hilti::Expression> u, shared_ptr<hilti::Expression> fn, shared_ptr<binpac::type::unit::Item> f, shared_ptr<hilti::Expression> top_level_s, bool op_is_set)
{
    shared_ptr<hilti::Instruction> ins_struct;
    shared_ptr<hilti::Instruction> ins_union;

    if ( op_is_set )
        ins_struct = ::hilti::instruction::struct_::IsSet;
    else
        ins_struct = ::hilti::instruction::struct_::Get;

    if ( op_is_set )
        ins_union = ::hilti::instruction::union_::IsSetField;
    else
        ins_union = ::hilti::instruction::union_::GetField;

    if ( _usingStructForItems(f) ) {
        auto sw = _switch(f);
        auto c = sw->case_(f);
        assert(c);
        auto s = _structTmp(cg, c->uniqueName());
        auto ufn = _structFieldName(c->uniqueName());

        if ( ! (f && f->attributes()->lookup("default")) ) {
            if ( op_is_set ) {
                _ifSet(cg, ::hilti::instruction::union_::IsSetField, u, ufn,
                       [&] () { // Union field in struct is set.
                           cg->builder()->addInstruction(s, ::hilti::instruction::union_::GetField, u, ufn);
                           _getFieldInStruct(cg, result, s, fn, nullptr, top_level_s, op_is_set); // TODO: For truly recursive fields, need new parent here.
                       },

                       [&] () { // Union field in struct is not set.
                           cg->builder()->addInstruction(result, ::hilti::instruction::operator_::Assign, ::hilti::builder::boolean::create(false));
                       });
            }

            else {
                cg->builder()->addInstruction(s, ::hilti::instruction::union_::GetField, u, ufn);
                _getFieldInStruct(cg, result, s, fn, nullptr, top_level_s, op_is_set); // TODO: For truly recursive fields, need new parent here.
            }
        }

        else {
            _ifSet(cg, ::hilti::instruction::union_::IsSetField, u, ufn,
                   [&] () { // Union field in struct is set.
                       cg->builder()->addInstruction(s, ::hilti::instruction::union_::GetField, u, ufn);
                       _getFieldInStruct(cg, result, s, fn, nullptr, top_level_s, op_is_set); // TODO: For truly recursive fields, need new parent here.
                   },

                   [&] () { // Union field in struct is not set.
                       auto dn = ::hilti::builder::string::create(::util::fmt("__default_%s", f->id()->name()));
                       cg->builder()->addInstruction(result, ins_struct, top_level_s, dn);
                   });
        }
    }

    else {
        if ( ! (f && f->attributes()->lookup("default")) ) {
                cg->builder()->addInstruction(result, ins_union, u, fn);
        }

        else {
            _ifSet(cg, ::hilti::instruction::union_::IsSetField, u, fn,
                   [&] () { // Union field in struct is set.
                       cg->builder()->addInstruction(result, ins_union, u, fn);
                   },

                   [&] () { // Union field in struct is not set.
                       auto dn = ::hilti::builder::string::create(::util::fmt("__default_%s", f->id()->name()));
                       cg->builder()->addInstruction(result, ins_struct, top_level_s, dn);
                   });
        }
    }
}

static void _presetDefaultInStruct(CodeGen* cg, shared_ptr<hilti::Expression> s, shared_ptr<hilti::Expression> fn, shared_ptr<hilti::Expression> value, shared_ptr<binpac::type::unit::Item> f, shared_ptr<hilti::Expression> top_level_s)
{
    if ( _switch(f) ) {
        auto dn = ::hilti::builder::string::create(::util::fmt("__default_%s", f->id()->name()));
        cg->builder()->addInstruction(hilti::instruction::struct_::Set, top_level_s, dn, value);
    }

    else
        cg->builder()->addInstruction(hilti::instruction::struct_::Set, s, fn, value);
}

static void _setFieldInStruct(CodeGen* cg, shared_ptr<hilti::Expression> s, shared_ptr<hilti::Expression> fn, shared_ptr<hilti::Expression> value, shared_ptr<binpac::type::unit::Item> f)
{
    if ( auto sw = _switch(f) ) {
        auto u = _unionTmp(cg, sw->uniqueName());
        auto ufn = _unionFieldName(sw->uniqueName());

        _ifSet(cg, ::hilti::instruction::struct_::IsSet, s, ufn,
               [&] () { // Struct field in union is set.
                   cg->builder()->addInstruction(u, ::hilti::instruction::struct_::Get, s, ufn);
               },

               [&] () { // Struct field in union is not set.
               });

        _setFieldInUnion(cg, u, fn, value, f);
        cg->builder()->addInstruction(::hilti::instruction::struct_::Set, s, ufn, u);
    }

    else
        cg->builder()->addInstruction(hilti::instruction::struct_::Set, s, fn, value);
}

static void _setFieldInUnion(CodeGen* cg, shared_ptr<hilti::Expression> u, shared_ptr<hilti::Expression> fn, shared_ptr<hilti::Expression> value, shared_ptr<binpac::type::unit::Item> f)
{
    auto sw = _switch(f);
    assert(sw);

    auto utype = _unionType(cg, sw->uniqueName());

    if ( _usingStructForItems(f) ) {
        auto c = sw->case_(f);
        assert(c);
        auto s = _structTmp(cg, c->uniqueName());
        auto sfn = _structFieldName(c->uniqueName());

        _ifSet(cg, ::hilti::instruction::union_::IsSetField, u, sfn,
               [&] () { // Struct field in union is set.
                   _getFieldInUnion(cg, s, u, sfn, nullptr, nullptr, false); // TODO: For truly recursive fields, need new parent here.
               },

               [&] () { // Struct field in union is not set.
                   cg->builder()->addInstruction(s, ::hilti::instruction::struct_::New,
                                                ::hilti::builder::type::create(_structType(cg, c->uniqueName())));
                   cg->builder()->addInstruction(u, ::hilti::instruction::operator_::Assign,
                                                ::hilti::builder::union_::create(utype, sfn, s));
               });

        _setFieldInStruct(cg, s, fn, value, nullptr);  // TODO: For truly recursive fields, need new parent here.
    }

    else {
        cg->builder()->addInstruction(u, ::hilti::instruction::operator_::Assign,
                                         ::hilti::builder::union_::create(utype, fn, value));
    }
}

static void _computeFieldPathInStruct(CodeGen* cg, shared_ptr<hilti::Type> s, shared_ptr<binpac::type::unit::Item> f, shared_ptr<binpac::ID> fn, ::hilti::builder::tuple::element_list* path)
{
    if ( auto rt = ast::tryCast<::hilti::type::Reference>(s) )
        s = rt->argType();

    s = cg->moduleBuilder()->resolveType(s);

    auto stype = ast::checkedCast<::hilti::type::Struct>(s);

    if ( auto sw = _switch(f) ) {
        auto ufn = _unionFieldNameAsString(sw->uniqueName());

        path->push_back(::hilti::builder::integer::create(stype->index(ufn)));
        _computeFieldPathInUnion(cg, stype->lookup(ufn)->type(), f, fn, path);
    }

    else
        path->push_back(::hilti::builder::integer::create(stype->index(fn->name())));
}

static void _computeFieldPathInUnion(CodeGen* cg, shared_ptr<hilti::Type> u, shared_ptr<binpac::type::unit::Item> f, shared_ptr<binpac::ID> fn, ::hilti::builder::tuple::element_list* path)
{
    u = cg->moduleBuilder()->resolveType(u);
    auto utype = ast::checkedCast<::hilti::type::Union>(u);

    if ( _usingStructForItems(f) ) {
        auto sw = _switch(f);
        auto c = sw->case_(f);
        assert(c);
        auto sfn = _structFieldNameAsString(c->uniqueName());

        path->push_back(::hilti::builder::integer::create(utype->index(sfn)));
        return _computeFieldPathInStruct(cg, utype->lookup(sfn)->type(), nullptr, fn, path);
    }

    else
        path->push_back(::hilti::builder::integer::create(utype->index(fn->name())));
}

shared_ptr<hilti::Expression> CodeGen::_hiltiItemOp(HiltiItemOp i, shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, const std::string& fname, shared_ptr<::hilti::Type> ftype, shared_ptr<hilti::Expression> addl_op)
{
    bool transient = false;

    if ( auto f = ast::tryCast<type::unit::item::Field>(field) )
        transient = f->transient();

    if ( field && field->aliased() )
        // Aliased fields are stored at the top-level, and clearing the field
        // here will direct the functions called be low to access it here.
        // The whole semantics of aliased fields aren't great though ...
        field = nullptr;

    auto fn = ::hilti::builder::string::create(fname);

    switch ( i ) {
     case GET: {
         if ( transient )
             return hiltiDefault(field->fieldType(), false, false);

         auto result = builder()->addTmp("item", ftype);
         _getFieldInStruct(this, result, unit, fn, field, unit, false);
         return result;
     }

     case GET_DEFAULT: {
         if ( _switch(field) )
             internalError("_hiltItemOp does not implement GET_DEFAULT for switches");

         if ( transient )
             return hiltiDefault(field->fieldType(), false, false);

         auto result = builder()->addTmp("item", ftype);
         builder()->addInstruction(result, hilti::instruction::struct_::GetDefault, unit, fn, addl_op);
         return result;
     }

     case IS_SET: {
         if ( transient )
             return ::hilti::builder::boolean::create(false);

         auto result = builder()->addTmp("is_set", ::hilti::builder::boolean::type());
         _getFieldInStruct(this, result, unit, fn, field, unit, true);
         return result;
     }

     case SET: {
         if ( ! transient )
             _setFieldInStruct(this, unit, fn, addl_op, field);

         return nullptr;
     }

     case PRESET_DEFAULT: {
         if ( ! transient )
             _presetDefaultInStruct(this, unit, fn, addl_op, field, unit);

         return nullptr;
     }

     case UNSET: {
         if ( _switch(field) )
             internalError("_hiltItemOp does not implement UNSET for switches");

         if ( ! transient )
             builder()->addInstruction(hilti::instruction::struct_::Unset, unit, fn);

         return nullptr;
     }

    }
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemGet(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, shared_ptr<hilti::Expression> default_)
{
    if ( default_ )
        return _hiltiItemOp(GET_DEFAULT, unit, field, field->id()->name(), hiltiType(field->fieldType()), default_);
    else
        return _hiltiItemOp(GET, unit, field, field->id()->name(), hiltiType(field->fieldType()), nullptr);
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemGet(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field, shared_ptr<::hilti::Type> ftype, shared_ptr<hilti::Expression> default_)
{
    return hiltiItemGet(unit, field->name(), ftype, default_);
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemGet(shared_ptr<hilti::Expression> unit, const string& field, shared_ptr<::hilti::Type> ftype, shared_ptr<hilti::Expression> default_)
{
    if ( default_ )
        return _hiltiItemOp(GET_DEFAULT, unit, nullptr, field, ftype, default_);
    else
        return _hiltiItemOp(GET, unit, nullptr, field, ftype, nullptr);
}

void CodeGen::hiltiItemSet(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, shared_ptr<hilti::Expression> value)
{
    _hiltiItemOp(SET, unit, field, field->id()->name(), value->type(), value);
}

void CodeGen::hiltiItemPresetDefault(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, shared_ptr<hilti::Expression> value)
{
    _hiltiItemOp(PRESET_DEFAULT, unit, field, field->id()->name(), value->type(), value);
}

void CodeGen::hiltiItemSet(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field, shared_ptr<hilti::Expression> value)
{
    hiltiItemSet(unit, field->name(), value);
}

void CodeGen::hiltiItemSet(shared_ptr<hilti::Expression> unit, const string& field, shared_ptr<hilti::Expression> value)
{
    _hiltiItemOp(SET, unit, nullptr, field, nullptr, value);
}

void CodeGen::hiltiItemUnset(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field)
{
    _hiltiItemOp(UNSET, unit, field, field->id()->name(), hiltiType(field->fieldType()), nullptr);
}

void CodeGen::hiltiItemUnset(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field)
{
    hiltiItemUnset(unit, field->name());
}

void CodeGen::hiltiItemUnset(shared_ptr<hilti::Expression> unit, const string& field)
{
    _hiltiItemOp(UNSET, unit, nullptr, field, nullptr, nullptr);
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemIsSet(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field)
{
    return _hiltiItemOp(IS_SET, unit, field, field->id()->name(), hiltiType(field->fieldType()), nullptr);
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemIsSet(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field)
{
    return hiltiItemIsSet(unit, field->name());
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemIsSet(shared_ptr<hilti::Expression> unit, const string& field)
{
    return _hiltiItemOp(IS_SET, unit, nullptr, field, nullptr, nullptr);
}

shared_ptr<hilti::Expression> CodeGen::hiltiIntPackFormat(int width, bool signed_, shared_ptr<binpac::Expression> byteorder)
{
    auto hltbo = byteorder ? hiltiExpression(byteorder) : hilti::builder::id::create("BinPAC::ByteOrder::Big");

    string big, little, host;

    switch ( width ) {
     case 8:
        if ( signed_ ) { little = "Int8Little"; big = "Int8Big"; host = "Int8"; }
        else           { little = "UInt8Little"; big = "UInt8Big"; host = "UInt8"; }
        break;

     case 16:
        if ( signed_ ) { little = "Int16Little"; big = "Int16Big"; host = "Int16"; }
        else           { little = "UInt16Little"; big = "UInt16Big"; host = "UInt16"; }
        break;

     case 32:
        if ( signed_ ) { little = "Int32Little"; big = "Int32Big"; host = "Int32"; }
        else           { little = "UInt32Little"; big = "UInt32Big"; host = "UInt32"; }
        break;

     case 64:
        if ( signed_ ) { little = "Int64Little"; big = "Int64Big"; host = "Int64"; }
        else           { little = "UInt64Little"; big = "UInt64Big"; host = "UInt64"; }
        break;

     default:
        internalError("unsupported bitwidth in _hiltiIntUnpackFormat()");
    }

    auto p1 = hilti::builder::id::create(string("Hilti::Packed::" + little));
    auto p2 = hilti::builder::id::create(string("Hilti::Packed::" + big));
    auto p3 = hilti::builder::id::create(string("Hilti::Packed::" + host));

    shared_ptr<hilti::ID> id;

    // FIXME: Manual constanst folding here for now ...

    if ( auto cbo = ast::tryCast<hilti::expression::Constant>(hltbo) ) {
        auto e = ast::tryCast<hilti::constant::Enum>(cbo->constant());
        if ( e )
            id = e->value();
    }

    else if ( auto eid = ast::tryCast<hilti::expression::ID>(hltbo) ) {
        id = eid->id();
    }

    if ( id ) {
        if ( id->pathAsString() == "BinPAC::ByteOrder::Little" )
            return p1;

        if ( id->pathAsString() == "BinPAC::ByteOrder::Big" )
            return p2;

        if ( id->pathAsString() == "BinPAC::ByteOrder::Host" )
            return p3;
    }

    auto t1 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Little"),
        p1 });

    auto t2 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Big"),
        p2 });

    auto t3 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Host"),
        p3 });

    auto tuple = hilti::builder::tuple::create({ t1, t2, t3 });
    auto result = moduleBuilder()->addTmp("fmt", hilti::builder::type::byName("Hilti::Packed"));
    builder()->addInstruction(result, hilti::instruction::Misc::SelectValue, hltbo, tuple);

    return result;
}

void CodeGen::_hiltiCreateParserInitFunction(shared_ptr<type::Unit> unit)
{
    string name = util::fmt("init_%s", unit->id()->name().c_str());
    auto void_ = hilti::builder::function::result(hilti::builder::void_::type());
    auto func = moduleBuilder()->pushFunction(name, void_, {});
    func->function()->setInitFunction();

    auto parser = hiltiParserDefinition(unit);
    auto funcs = builder()->addTmp("funcs", hilti::builder::tuple::type({ hilti::builder::caddr::type(), hilti::builder::caddr::type() }));

    auto module = unit->firstParent<Module>();
    assert(module);

    auto fname = ::util::fmt("%s::%s", module->id()->name(), unit->id()->pathAsString(module->id()));

    hilti::builder::list::element_list mtypes;
    hilti::builder::list::element_list ports;

    auto props_mtype = unit->properties("mimetype");
    auto props_port = unit->properties("port");

    for ( auto p : props_mtype )
        mtypes.push_back(hiltiExpression(p->property()->value()));

    for ( auto p : props_port )
        ports.push_back(hiltiExpression(p->property()->value()));

    auto hilti_mtypes = hilti::builder::list::create(hilti::builder::string::type(), mtypes, unit->location());
    auto hilti_ports  = hilti::builder::list::create(hilti::builder::port::type(), ports, unit->location());

    auto descr = unit->property("description");

    shared_ptr<hilti::Expression> hilti_descr = nullptr;

    if ( descr )
        hilti_descr = hiltiExpression(descr->property()->value());
    else
        hilti_descr = hilti::builder::string::create("No description.");

    builder()->addInstruction(parser, hilti::instruction::struct_::New,
                              hilti::builder::id::create("BinPACHilti::Parser"));

    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("name"),
                              hilti::builder::string::create(fname));

    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("description"),
                              hilti_descr);

    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("ports"),
                              hilti_ports);

    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("params"),
                              hilti::builder::integer::create(unit->parameters().size()));

    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("mime_types"),
                              hilti_mtypes);

    auto f = builder()->addTmp("f", hilti::builder::caddr::type());

    auto caddr_nulls = hilti::builder::tuple::create({ hilti::builder::caddr::create(), hilti::builder::caddr::create() });

    // Standard parse functions.

    if ( options().generate_parsers )
        builder()->addInstruction(funcs, hilti::instruction::caddr::Function, _parser_builder->hiltiCreateHostFunction(unit, false));
    else
        builder()->addInstruction(funcs, hilti::instruction::operator_::Assign, caddr_nulls);

    builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("parse_func"), f);

    builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(1));
    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("resume_func"), f);

    // Compose functions.

    if ( options().generate_composers )
        builder()->addInstruction(funcs, hilti::instruction::caddr::Function, _composer->hiltiCreateHostFunction(unit));
    else
        builder()->addInstruction(funcs, hilti::instruction::operator_::Assign, caddr_nulls);

    builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("compose_func"), f);

    // Sink parse functions.
    if ( options().generate_parsers )
        builder()->addInstruction(funcs, hilti::instruction::caddr::Function, _parser_builder->hiltiCreateHostFunction(unit, true));
    else
        builder()->addInstruction(funcs, hilti::instruction::operator_::Assign, caddr_nulls);

    builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("parse_func_sink"), f);

    builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(1));
    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("resume_func_sink"), f);


    // Allocation function.

    if ( options().generate_parsers )
        builder()->addInstruction(funcs, hilti::instruction::caddr::Function, hiltiFunctionNew(unit));
    else
        builder()->addInstruction(funcs, hilti::instruction::operator_::Assign, caddr_nulls);

    builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    builder()->addInstruction(hilti::instruction::struct_::Set,
                              parser,
                              hilti::builder::string::create("new_func"),
                              f);

    // "type_info" is initialized by "BinPACHilti::register_parser".

    auto ti = hilti::builder::type::create(hiltiTypeParseObject(unit));

    builder()->addInstruction(hilti::instruction::flow::CallVoid,
                              hilti::builder::id::create("BinPACHilti::register_parser"),
                              hilti::builder::tuple::create( { parser, ti } ));

    builder()->addInstruction(hilti::instruction::flow::ReturnVoid);

    moduleBuilder()->popFunction();
}

shared_ptr<hilti::Expression> CodeGen::hiltiItemComputePath(shared_ptr<hilti::Type> unit, shared_ptr<binpac::type::unit::Item> f)
{
    auto id = f->id();

    if ( f && f->aliased() )
        f = nullptr;

    ::hilti::builder::tuple::element_list path;
    _computeFieldPathInStruct(this, unit, f, id, &path);
    return ::hilti::builder::tuple::create(path);
}

shared_ptr<hilti::Expression> CodeGen::hiltiParserDefinition(shared_ptr<type::Unit> unit)
{
    auto unit_module = unit->firstParent<Module>();
    auto scope = unit_module && unit_module->id()->name() != moduleBuilder()->module()->id()->name()
        ? unit_module->id()->name() : string();

    string name = scope.size() ? util::fmt("%s::__binpac_parser_%s", scope, unit->id()->name().c_str())
                               : util::fmt("__binpac_parser_%s", unit->id()->pathAsString().c_str());

    auto parser = ast::tryCast<hilti::Expression>(moduleBuilder()->lookupNode("parser-definition", name));

    if ( parser )
        return parser;

    auto t = hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Parser"));

    if ( scope.size() )
        parser = moduleBuilder()->declareGlobal(name, t);
    else
        parser = moduleBuilder()->addGlobal(name, t);

    moduleBuilder()->cacheNode("parser-definition", name, parser);
    return parser;
}
