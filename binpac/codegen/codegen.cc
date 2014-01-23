
#include <hilti/hilti.h>

#include "codegen.h"
#include "../context.h"
#include "../options.h"

#include "code-builder.h"
#include "parser-builder.h"
#include "type-builder.h"
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
    _imported_types.clear();

    _code_builder = unique_ptr<codegen::CodeBuilder>(new codegen::CodeBuilder(this));
    _parser_builder = unique_ptr<codegen::ParserBuilder>(new codegen::ParserBuilder(this));
    _type_builder = unique_ptr<codegen::TypeBuilder>(new codegen::TypeBuilder(this));
    _coercer = unique_ptr<Coercer>(new Coercer());

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

        _module = module;
        _code_builder->processOne(module);

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

                auto hid = hiltiID(id);
                if ( ! moduleBuilder()->declared(hid) )
                    moduleBuilder()->addType(hid, hlttype);

                done.insert(t.first);
            }
        }

#if 0
        _mbuilder->module()->compilerContext()->dump(_mbuilder->module(), std::cerr);
        _mbuilder->module()->compilerContext()->print(_mbuilder->module(), std::cerr);
#endif
        _module = nullptr;

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
    return _parser_builder->unit();
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

    if ( ! uid->isScoped() || uid->scope() != mod->id()->name() ) {
        if ( mod->id()->name() != module()->id()->name() )
            name = util::fmt("%s::%s", mod->id()->name(), uid->pathAsString());
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

    func = _parser_builder->hiltiCreateParseFunction(u);

    _mbuilder->cacheNode("parse-func", grammar->name(), func);
    return func;
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeParseObject(shared_ptr<type::Unit> unit)
{
    auto t = ast::tryCast<hilti::Type>(_mbuilder->lookupNode("parse-obj", unit->id()->name()));

    if ( t )
        return t;

    t = _parser_builder->hiltiTypeParseObject(unit);

    _mbuilder->cacheNode("parse-obj", unit->id()->name(), t);
    return t;
}

shared_ptr<hilti::Type> CodeGen::hiltiTypeParseObjectRef(shared_ptr<type::Unit> u)
{
    return hilti::builder::reference::type(hiltiTypeParseObject(u));
}

void CodeGen::hiltiExportParser(shared_ptr<type::Unit> unit)
{
    _parser_builder->hiltiExportParser(unit);
}

void CodeGen::hiltiUnitHooks(shared_ptr<type::Unit> unit)
{
    _parser_builder->hiltiUnitHooks(unit);
}

void CodeGen::hiltiRunFieldHooks(shared_ptr<type::unit::Item> item, shared_ptr<hilti::Expression> self)
{
    _parser_builder->hiltiRunFieldHooks(item, self);
}

void CodeGen::hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook)
{
   _parser_builder->hiltiDefineHook(id, hook);
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

    // TODO: Do we always want to export these?
    if ( ! declare_only )
        moduleBuilder()->exportID(name);

    if ( func->body() && ! declare_only ) {
        auto decl = moduleBuilder()->pushFunction(name, result, params, cc, nullptr, false, func->location());
        hiltiStatement(func->body());
        moduleBuilder()->popFunction();
        return decl;
    }

    else
        return moduleBuilder()->declareFunction(name, result, params, cc, func->location());
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
    auto name = util::fmt("%s__%s", id, util::uitoa_n(util::hash(type), 64, 4));

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

    for ( auto a : args )
        hilti_arg_list.push_back(hiltiExpression(a));

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
    return _parser_builder->hiltiSelf();
}

shared_ptr<hilti::Expression> CodeGen::hiltiCookie()
{
    return _parser_builder->hiltiCookie();
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
    return _parser_builder->hiltiFunctionNew(unit);
}

shared_ptr<hilti::Expression> CodeGen::hiltiCastEnum(shared_ptr<hilti::Expression> val, shared_ptr<hilti::Type> dst)
{
    auto i = builder()->addTmp("eval", hilti::builder::integer::type(64));
    auto e = builder()->addTmp("enum", dst);
    builder()->addInstruction(i, hilti::instruction::enum_::ToInt, val);
    builder()->addInstruction(e, hilti::instruction::enum_::FromInt, i);
    return e;
}

void CodeGen::hiltiWriteToSinks(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> data)
{
    _parser_builder->hiltiWriteToSinks(field, data);
}

void CodeGen::hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> data)
{
    _parser_builder->hiltiWriteToSink(sink, data);
}

void CodeGen::hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> begin, shared_ptr<hilti::Expression> end)
{
    auto data = builder()->addTmp("data", hilti::builder::reference::type(hilti::builder::bytes::type()));
    builder()->addInstruction(data, hilti::instruction::bytes::Sub, begin, end);
    return hiltiWriteToSink(sink, data);
}

shared_ptr<hilti::Expression> CodeGen::hiltiApplyAttributesToValue(shared_ptr<hilti::Expression> val, shared_ptr<AttributeSet> attrs)
{
    auto convert = attrs->lookup("convert");

    if ( convert ) {
        hiltiBindDollarDollar(val);
        auto dd = builder()->addTmp("__dollardollar", val->type());
        builder()->addInstruction(dd, hilti::instruction::operator_::Assign, val);
        val = hiltiExpression(convert->value());
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

shared_ptr<hilti::Expression> CodeGen::hiltiExtractsBitsFromInteger(shared_ptr<hilti::Expression> value, shared_ptr<Type> type, shared_ptr<hilti::Expression> lower_in, shared_ptr<hilti::Expression> upper_in)
{
    auto itype = ast::checkedCast<type::Integer>(type);
    auto hitype = hiltiType(itype);
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
    auto eval = hiltiExpression(itype->bitOrder(), etype);

    builder()->addSwitch(eval, bdefault, cases);

    moduleBuilder()->pushBuilder(done);

    //

    builder()->addInstruction(result, hilti::instruction::integer::Mask, value, lower, upper);

    return result;
}
