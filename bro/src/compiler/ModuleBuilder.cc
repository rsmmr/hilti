
#include <Traverse.h>
#include <Func.h>
#undef List

#include <list>

#include <util/util.h>

#include "Compiler.h"
#include "ModuleBuilder.h"
#include "ValueBuilder.h"
#include "ExpressionBuilder.h"
#include "StatementBuilder.h"
#include "TypeBuilder.h"
#include "ConversionBuilder.h"
#include "ASTDumper.h"
#include "../LocalReporter.h"
#include "../Converter.h"

// If we defined, we compile only a small subset of symbols:
//    - all event handlers with priority 42.
//    - all script functions with names starting with "foo".
//    - no global symbols defined outside of the global namespace.
//
// This is for testing as long as we can't compile everything yet.
#define LIMIT_COMPILED_SYMBOLS 1

namespace BifConst { namespace Hilti {  extern int save_hilti;  }  }

using namespace bro::hilti::compiler;

std::shared_ptr<ModuleBuilderCallback> ModuleBuilder::callback;

class bro::hilti::compiler::ModuleBuilderCallback : public TraversalCallback {
public:
	ModuleBuilderCallback();

	bool Traverse();

	std::list<const Func *> Functions(const string& ns);
	std::list<const ::ID *> Globals(const string& ns);

protected:
	TraversalCode PreID(const ::ID* id) override;

private:
	typedef std::set<const Func *> function_set;
	typedef std::list<const Func *> function_list;
	typedef std::list<const ::ID *> id_list;
	typedef std::map<string, std::shared_ptr<id_list>> ns_map;
	ns_map namespaces;
};

ModuleBuilderCallback::ModuleBuilderCallback()
	{
	}

std::list<const Func *> ModuleBuilderCallback::Functions(const string& ns)
	{
	function_list functions;
	function_set sfunctions;

	auto n = namespaces.find(ns);

	if ( n == namespaces.end() )
		return functions;

	for ( auto id : *(*n).second )
		{
		if ( ! id->ID_Val() )
			continue;

		if ( id->ID_Val()->Type()->Tag() == TYPE_FUNC )
			{
			auto func = id->ID_Val()->AsFunc();
			assert(func);
			sfunctions.insert(func);
			}
		}

	for ( auto f : sfunctions )
		functions.push_back(f);

	functions.sort([](const Func* a, const Func* b) -> bool {
		return strcmp(a->Name(), b->Name()) < 0;
	});

	return functions;
	}

std::list<const ::ID *> ModuleBuilderCallback::Globals(const string& ns)
	{
	std::list<const ::ID *> globals;
	std::set<const ::ID *>  sglobals;

#if LIMIT_COMPILED_SYMBOLS
	if ( ns != "GLOBAL" )
		// FIXME: Remove once we can compile all globals.
		return globals;
#endif

	auto n = namespaces.find(ns);

	if ( n == namespaces.end() )
		return globals;

	for ( auto id : *(*n).second )
		{
		if ( ! id->IsGlobal() )
			continue;

		if ( id->IsConst() )
			continue;

		if ( id->HasVal() && id->ID_Val()->Type()->Tag() == TYPE_FUNC )
			continue;

		if ( id->AsType() )
			continue;

		sglobals.insert(id);
		}

	for ( auto id : sglobals )
		globals.push_back(id);

	globals.sort([](const ::ID* a, const ::ID* b) -> bool {
		return strcmp(a->Name(), b->Name()) < 0;
	});

	return globals;
	}

TraversalCode ModuleBuilderCallback::PreID(const ::ID* id)
	{
	if ( ! id->IsGlobal() )
		return TC_CONTINUE;

	auto n = namespaces.find(id->ModuleName());

	if ( n == namespaces.end() )
		n = namespaces.insert(std::make_pair(id->ModuleName(), std::make_shared<id_list>())).first;

	n->second->push_back(id);
	return TC_CONTINUE;
	}

ModuleBuilder::ModuleBuilder(class Compiler* arg_compiler, shared_ptr<::hilti::CompilerContext> ctx, const std::string& namespace_)
	: BuilderBase(this), ::hilti::builder::ModuleBuilder(ctx, namespace_)
	{
	if ( ! callback )
		{
		callback = std::make_shared<ModuleBuilderCallback>();
		traverse_all(callback.get());
		}

	ns = namespace_;
	compiler = arg_compiler;
	expression_builder = new class ExpressionBuilder(this);
	statement_builder = new class StatementBuilder(this);
	type_builder = new class TypeBuilder(this);
	value_builder = new class ValueBuilder(this);
	conversion_builder = new class ConversionBuilder(this);

	importModule("Hilti");
	importModule("LibBro");
	}

ModuleBuilder::~ModuleBuilder()
	{
	delete expression_builder;
	delete statement_builder;
	delete type_builder;
	delete value_builder;
	}

shared_ptr<::hilti::Module> ModuleBuilder::Compile()
	{
#ifdef DEBUG
	if ( ns.size() )
		DBG_LOG_COMPILER("Beginning to compile namespace '%s'", ns.c_str());
	else
		DBG_LOG_COMPILER("Beginning to compile global namespace");
#endif

	try
		{
		for ( auto id : callback->Globals(ns) )
			{
			auto type = id->Type();
			auto init = id->HasVal() ? HiltiValue(id->ID_Val()) : HiltiInitValue(type);
			addGlobal(HiltiSymbol(id), HiltiType(type), init);
			}

		for ( auto f : callback->Functions(ns) )
			CompileFunction(f);
		}

	catch ( const BuilderException& e )
		{
		reporter::error(e.what());
		}

	DBG_LOG_COMPILER("Done compiling namespace %s", ns.c_str());

	if ( reporter::errors() > 0 )
		return nullptr;

	// We save the source here once so that we have it on disk for inspection
	// in case finalization/compilation fails. Normally the Manager will
	// later override it with the final version.
	//
	// TODO: We need to unify the save_* code. It should probably always
	// saved as quickly as possible.
	//
	if ( BifConst::Hilti::save_hilti )
		saveHiltiCode(::util::fmt("bro.%s.hlt", module()->id()->name()));

	return finalize();
	}

class Compiler* ModuleBuilder::Compiler() const
	{
	return compiler;
	}

class ExpressionBuilder* ModuleBuilder::ExpressionBuilder() const
	{
	return expression_builder;
	}

class StatementBuilder* ModuleBuilder::StatementBuilder() const
	{
	return statement_builder;
	}

class TypeBuilder* ModuleBuilder::TypeBuilder() const
	{
	return type_builder;
	}

class ValueBuilder* ModuleBuilder::ValueBuilder() const
	{
	return value_builder;
	}

class ::bro::hilti::compiler::ConversionBuilder* ModuleBuilder::ConversionBuilder() const
	{
	return conversion_builder;
	}

string ModuleBuilder::Namespace() const
	{
	return ns;
	}

void ModuleBuilder::CompileFunction(const Func* func)
	{
#if LIMIT_COMPILED_SYMBOLS
	if ( string(func->Name()) != "bro_init" && string(func->Name()) != "to_lower" && strncmp(func->Name(), "foo", 3) != 0 )
		return;
#endif

	if ( func->GetKind() == Func::BUILTIN_FUNC )
		// We build wrappers in DeclareFunction().
		return;

	functions.push_back(func);

	auto bfunc = static_cast<const BroFunc*>(func);

	switch ( bfunc->FType()->Flavor() ) {
		case FUNC_FLAVOR_FUNCTION:
			CompileScriptFunction(bfunc);
			break;

		case FUNC_FLAVOR_EVENT:
			CompileEvent(bfunc);
			break;

		case FUNC_FLAVOR_HOOK:
			CompileHook(bfunc);
			break;

		default:
			reporter::internal_error("unknown function flavor");
	}

	functions.pop_back();
	}

void ModuleBuilder::CompileScriptFunction(const BroFunc* func)
	{
	if ( func->GetBodies().size() == 0 )
		return;

	DBG_LOG_COMPILER("Compiling script function %s", func->Name());

	auto fname = Compiler()->HiltiSymbol(func, module());
	auto ftype = HiltiFunctionType(func->FType());

	assert(func->GetBodies().size() == 1);
	auto body = func->GetBodies()[0];

	auto hfunc = pushFunction(::hilti::builder::id::node(fname),
				 ast::checkedCast<::hilti::type::Function>(ftype),
				 nullptr);

	hfunc->addComment(func->Name());

	CompileFunctionBody(func, &body);

	popFunction();
	}

void ModuleBuilder::CompileEvent(const BroFunc* event)
	{
	DBG_LOG_COMPILER("Compiling event function %s", event->Name());

	auto hook_name = Compiler()->HiltiSymbol(event, module());
	auto stub_name = Compiler()->HiltiStubSymbol(event, module(), false);
	auto type = HiltiFunctionType(event->FType());

	for ( auto b : event->GetBodies() )
		{
#if LIMIT_COMPILED_SYMBOLS
		if ( b.priority != 42 )
			// FIXME: Hack to select which handlers we want to compile.
			continue;
#endif

		// Compile the event body into a hook.

		auto attrs = ::hilti::builder::hook::attributes();

		if ( b.priority )
			attrs.push_back(::hilti::builder::hook::hook_attribute("&priority", b.priority));

		auto hook = pushHook(::hilti::builder::id::node(hook_name),
			 ast::checkedCast<::hilti::type::Hook>(type),
			 nullptr, attrs);

		hook->addComment(Location(event));

		CompileFunctionBody(event, &b);

		popHook();
		}

	// If there's no body, make sure the hook is declared.
	if ( event->GetBodies().empty() )
		DeclareEvent(event);

	// Create the stub function that takes Bro Vals and returns a
	// callable to execute the hook.

	auto stub_result = ::hilti::builder::function::result(::hilti::builder::void_::type());
	auto stub_args = ::hilti::builder::function::parameter_list();

	RecordType* args = event->FType()->Args();
	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");

	for ( int i = 0; i < args->NumFields(); i++ )
		{
		auto name = ::hilti::builder::id::node(::util::fmt("arg%d", i+1));
		auto arg = ::hilti::builder::function::parameter(name, vtype, false, nullptr);
		stub_args.push_back(arg);
		}

	exportID(stub_name);
	auto stub = pushFunction(stub_name, stub_result, stub_args, ::hilti::type::function::HILTI_C);

	::hilti::builder::tuple::element_list hook_args;

	for ( int i = 0; i < args->NumFields(); i++ )
		{
		auto arg = ::hilti::builder::id::create(::util::fmt("arg%d", i+1));
		auto btype = args->FieldType(i);
		auto harg = RuntimeValToHilti(arg, btype);
		hook_args.push_back(harg);
		}

	auto tc = ::hilti::builder::callable::type(::hilti::builder::void_::type());
	auto rtc = ::hilti::builder::reference::type(tc);
	auto c = addTmp("c", rtc);
	auto t = ::hilti::builder::tuple::create(hook_args);

#if 0
	Builder()->addInstruction(::hilti::instruction::hook::Run,
				  ::hilti::builder::id::create(hook_name),
				  t);
#else
	Builder()->addInstruction(c,
				  ::hilti::instruction::callable::NewHook,
				  ::hilti::builder::type::create(tc),
				  ::hilti::builder::id::create(hook_name),
				  t);
#endif

	// TODO: We this should queue the callable, not directl run it.
	Builder()->addInstruction(::hilti::instruction::flow::CallCallableVoid, c);

	popFunction();

	// compilerContext()->print(module(), std::cerr);
	}

void ModuleBuilder::CompileHook(const BroFunc* hook)
	{
	DBG_LOG_COMPILER("Compiling hook function %s", hook->Name());
	}

void ModuleBuilder::CompileFunctionBody(const ::Func* func, void* vbody)
	{
	// Note: we can't rely on the function's scope to understand which
	// local we have. For events, it seems that all locals from any
	// handler end up in the same scope. Furthermore, not all local are
	// represented in the scope; some may be introduced only through
	// InitStmts.

	auto body = reinterpret_cast<::Func::Body*>(vbody);
	StatementBuilder()->Compile(body->stmts);
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareFunction(const Func* func)
	{
	if ( func->GetKind() == Func::BUILTIN_FUNC )
		{
		Error("No implementation of DeclareFunction for BiFs; use CallFunction() to call it.", func);
		return 0;
		}

	auto bfunc = static_cast<const BroFunc*>(func);

	switch ( bfunc->FType()->Flavor() ) {
		case FUNC_FLAVOR_FUNCTION:
			return DeclareScriptFunction(bfunc);

		case FUNC_FLAVOR_EVENT:
			return DeclareEvent(bfunc);

		case FUNC_FLAVOR_HOOK:
			return DeclareHook(bfunc);

		default:
			reporter::internal_error("unknown function flavor in DeclareFunction");
	}

	reporter::internal_error("cannot be reached");
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareScriptFunction(const ::BroFunc* func)
	{
	auto name = Compiler()->HiltiSymbol(func, module());
	bool anon = (string(func->Name()) == "anonymous-function");

	// If the function is part of the same module, we'll compile it
	// separately.
	if ( ::extract_module_name(func->Name()) == module()->id()->name() && ! anon )
		{
#if LIMIT_COMPILED_SYMBOLS
	if ( ! (string(func->Name()) != "bro_init" && string(func->Name()) != "to_lower" && strncmp(func->Name(), "foo", 3) != 0) )
#endif
		return ::hilti::builder::id::create(name);
		}

	// We can have multiple "anonymous-function"s.
	if ( anon )
		name += ::util::fmt("_%p", func);

	auto id = ::hilti::builder::id::create(name);
	auto type = HiltiFunctionType(func->FType());

	declareFunction(::hilti::builder::id::node(name), ast::checkedCast<::hilti::type::Function>(type));
	return id;
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareEvent(const ::BroFunc* event)
	{
	auto name = Compiler()->HiltiSymbol(event, module());
	auto type = HiltiFunctionType(event->FType());

	declareHook(::hilti::builder::id::node(name), ast::checkedCast<::hilti::type::Hook>(type));

	auto handler = ::util::fmt("__bro_handler_%s", name);
	auto bro_handler = addGlobal(handler, ::hilti::builder::type::byName("LibBro::BroEventHandler"));

	return ::hilti::builder::id::create(handler);
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareHook(const ::BroFunc* event)
	{
	Error("ModuleBuilder::DeclareHook not yet implemented");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ModuleBuilder::DeclareGlobal(const ::ID* id)
	{
	assert(id->IsGlobal() && ! id->AsType());

	auto symbol = HiltiSymbol(id);

	// std::cerr << id->Name() << " " << symbol << " " << id->ModuleName() << " " << module()->id()->name() << std::endl;

	// Only need to actually declare it if it's in a different module;
	// for the current module we'll generate the global variabel itself
	// elsewhere.
	if ( id->ModuleName() != module()->id()->name() )
		declareGlobal(symbol, HiltiType(id->Type()));

	return ::hilti::builder::id::create(symbol);
	}

std::shared_ptr<::hilti::Expression> ModuleBuilder::DeclareLocal(const ::ID* id)
	{
	assert(! id->IsGlobal() && ! id->AsType());

	auto symbol = HiltiSymbol(id);

	if ( ! hasLocal(symbol) )
		{
		auto type = id->Type();
		auto init_val = id->HasVal() ? HiltiValue(id->ID_Val()) : HiltiInitValue(type);
		addLocal(symbol, HiltiType(type), init_val);
		}

	return ::hilti::builder::id::create(symbol);
	}

const ::Func* ModuleBuilder::CurrentFunction() const
	{
	return functions.size() ? functions.back() : nullptr;
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallFunction(const ::Expr* func, ListExpr* args)
	{
	if ( func->Tag() == EXPR_NAME )
		{
		auto id = func->AsNameExpr()->Id();

		if ( id->Type()->Tag() == TYPE_FUNC )
			{
			auto f = id->ID_Val()->AsFunc();

			if ( f->GetKind() == Func::BUILTIN_FUNC )
				return HiltiCallBuiltinFunction(static_cast<const BuiltinFunc*>(f), args);
			}
		}

	auto ftype = func->Type()->AsFuncType();

	switch ( ftype->Flavor() ) {
		case FUNC_FLAVOR_FUNCTION:
			return HiltiCallScriptFunction(func, args);

		case FUNC_FLAVOR_EVENT:
			return HiltiCallEvent(func, args);

		case FUNC_FLAVOR_HOOK:
			return HiltiCallHook(func, args);

		default:
			reporter::internal_error("unknown function flavor in CallFunction");
	}

	reporter::internal_error("cannot be reached");
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallScriptFunction(const ::Expr* func, ListExpr* args)
	{
	auto ftype = func->Type()->AsFuncType();
	auto ytype = ftype->YieldType();

	auto hfunc = HiltiExpression(func);
	auto hargs = HiltiExpression(args, ftype->ArgTypes());

	if ( ytype && ytype->Tag() != TYPE_VOID && ytype->Tag() != TYPE_ANY )
		{
		auto result = Builder()->addTmp("result", HiltiType(ftype->YieldType()));
		Builder()->addInstruction(result, ::hilti::instruction::flow::CallResult, hfunc, hargs);
		return result;
		}

	else
		{
		Builder()->addInstruction(::hilti::instruction::flow::CallVoid, hfunc, hargs);
		return nullptr;
		}
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallEvent(const ::Expr* func, ListExpr* args)
	{
	Error("CallEvent not yet implemented");
	return 0;
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallHook(const ::Expr* func, ListExpr* args)
	{
	Error("CallHook not yet implemented");
	return 0;
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallBuiltinFunction(const ::BuiltinFunc* bif, ListExpr* args)
	{
	std::string bif_symbol;
	auto symbol = HiltiSymbol(bif);

	// See if we have a statically compiled bif function available.
	if ( Compiler()->HaveHiltiBif(bif->Name(), &bif_symbol) )
		return HiltiCallBuiltinFunctionHILTI(bif, args);
	else
		return HiltiCallBuiltinFunctionLegacy(bif, args);
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallBuiltinFunctionHILTI(const ::BuiltinFunc* bif, ListExpr* args)
	{
	auto ftype = bif->FType();
	auto ytype = ftype->YieldType();

	auto hftype = ::ast::checkedCast<::hilti::type::Function>(HiltiType(ftype));
	hftype->setCallingConvention(::hilti::type::function::HILTI_C);

	string bif_symbol;
	auto have = Compiler()->HaveHiltiBif(bif->Name(), &bif_symbol);
	assert(have);

	declareFunction(bif_symbol, hftype);

	auto hfunc = ::hilti::builder::id::create(bif_symbol);
	auto hargs = HiltiExpression(args, ftype->ArgTypes());

	if ( ytype && ytype->Tag() != TYPE_VOID && ytype->Tag() != TYPE_ANY )
		{
		auto result = Builder()->addTmp("bif_result", HiltiType(ytype));
		Builder()->addInstruction(result, ::hilti::instruction::flow::CallResult, hfunc, hargs);
		return result;
		}

	else
		{
		Builder()->addInstruction(::hilti::instruction::flow::CallVoid, hfunc, hargs);
		return nullptr;
		}
	}

shared_ptr<::hilti::Expression> ModuleBuilder::HiltiCallBuiltinFunctionLegacy(const ::BuiltinFunc* bif, ListExpr* args)
	{
	if ( ! bif->TheFunc() )
		{
		// This should be a HILTI BiF, however then we shouldn't arrive here.
		auto bif_symbol = HiltiSymbol(bif);
		fatalError(::util::fmt("Function %s declared but not implemented (bif symbol %s)", bif->Name(), bif_symbol));
		}

	RecordType* fargs = bif->FType()->Args();
	auto exprs = args->Exprs();

	::hilti::builder::tuple::element_list func_args;

        // This is apparently how a varargs function is defined in Bro ...
	bool var_args = (fargs->NumFields() == 1 && fargs->FieldType(0)->Tag() == TYPE_ANY);

	loop_over_list(exprs, i)
		{
		auto arg = HiltiExpression(exprs[i]);
		// auto btype = var_args ? exprs[i]->Type() : fargs->FieldType(i);
		auto val = RuntimeHiltiToVal(arg, exprs[i]->Type());
		func_args.push_back(val);
		}

	auto f = ::hilti::builder::bytes::create(bif->Name());
	auto t = ::hilti::builder::tuple::create(func_args);
	auto ca = ::hilti::builder::tuple::create({ f, t });

	auto ytype = bif->FType()->YieldType();
	auto hytype = HiltiType(ytype);

	if ( ytype && ytype->Tag() != TYPE_VOID && ytype->Tag() != TYPE_ANY )
		{
		auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");
		auto rval = addTmp("rval", vtype);

		Builder()->addInstruction(rval, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::call_bif_result"), ca);

		return RuntimeValToHilti(rval, ytype);
		}

	else
		{
		Builder()->addInstruction(::hilti::instruction::flow::CallVoid,
					  ::hilti::builder::id::create("LibBro::call_bif_void"), ca);
		return nullptr;
		}
	}

