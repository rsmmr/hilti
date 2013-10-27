
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

namespace BifConst { namespace Hilti {  extern int save_hilti;  }  }

using namespace bro::hilti::compiler;

std::shared_ptr<ModuleBuilderCallback> ModuleBuilder::callback;

class bro::hilti::compiler::ModuleBuilderCallback : public TraversalCallback {
public:
	ModuleBuilderCallback();

	bool Traverse();

	std::list<const Func *> Functions(const string& ns);

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
	// FIXME.
	if ( string(func->Name()) != "bro_init" && string(func->Name()) != "to_lower" )
		return;

	if ( func->GetKind() == Func::BUILTIN_FUNC )
		// We build wrappers in DeclareFunction().
		return;

	auto bfunc = static_cast<const BroFunc*>(func);

	switch ( bfunc->FType()->Flavor() ) {
		case FUNC_FLAVOR_FUNCTION:
			return CompileScriptFunction(bfunc);

		case FUNC_FLAVOR_EVENT:
			return CompileEvent(bfunc);

		case FUNC_FLAVOR_HOOK:
			return CompileHook(bfunc);

		default:
			reporter::internal_error("unknown function flavor");
	}

	reporter::internal_error("cannot be reached");
	}

void ModuleBuilder::CompileScriptFunction(const BroFunc* func)
	{
	DBG_LOG_COMPILER("Compiling script function %s", func->Name());
	}

void ModuleBuilder::CompileEvent(const BroFunc* event)
	{
	DBG_LOG_COMPILER("Compiling event function %s", event->Name());

	auto hook_name = Compiler()->HiltiSymbol(event, module());
	auto stub_name = Compiler()->HiltiStubSymbol(event, module(), false);
	auto type = HiltiType(event->FType());

	for ( auto b : event->GetBodies() )
		{
		if ( b.priority != 42 )
			// FIXME: Hack to select which handlers we want to compile.
			continue;

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
	auto body = reinterpret_cast<::Func::Body*>(vbody);

	// TODO: Create local variables.

	StatementBuilder()->Compile(body->stmts);
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareFunction(const Func* func)
	{
	if ( func->GetKind() == Func::BUILTIN_FUNC )
		return DeclareBuiltinFunction(static_cast<const BuiltinFunc*>(func));

	auto bfunc = static_cast<const BroFunc*>(func);

	switch ( bfunc->FType()->Flavor() ) {
		case FUNC_FLAVOR_FUNCTION:
			return DeclareScriptFunction(bfunc);

		case FUNC_FLAVOR_EVENT:
			return DeclareEvent(bfunc);

		case FUNC_FLAVOR_HOOK:
			return DeclareHook(bfunc);

		default:
			reporter::internal_error("unknown function flavor");
	}

	reporter::internal_error("cannot be reached");
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareScriptFunction(const ::BroFunc* func)
	{
	Error("ModuleBuilder::DeclareScriptFunction not yet implemented");
	return nullptr;
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareEvent(const ::BroFunc* event)
	{
	auto name = Compiler()->HiltiSymbol(event, module());
	auto type = HiltiType(event->FType());

	declareHook(::hilti::builder::id::node(name), ast::checkedCast<::hilti::type::Hook>(type));
	return ::hilti::builder::id::create(name);
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareHook(const ::BroFunc* event)
	{
	Error("ModuleBuilder::DeclareHook not yet implemented");
	return nullptr;
	}

shared_ptr<::hilti::Expression> ModuleBuilder::DeclareBuiltinFunction(const ::BuiltinFunc* bif)
	{
	auto ftype = ::ast::checkedCast<::hilti::type::Function>(HiltiType(bif->FType()));

	auto symbol = HiltiSymbol(bif);
	std::string bif_symbol;

	// See if we have a statically compiled bif function available.
	if ( Compiler()->HaveHiltiBif(bif->Name(), &bif_symbol) )
		{
		ftype->setCallingConvention(::hilti::type::function::HILTI_C);
		declareFunction(bif_symbol, ftype);
		return ::hilti::builder::id::create(bif_symbol);
		}

	// We shouldn't arrive here for a HILTI-level BiF.
	if ( ! bif->TheFunc() )
		fatalError(::util::fmt("Function %s declared but not implemented (bif symbol %s)", bif->Name(), bif_symbol));

	// Add a stub function into the glue code that calls Bro's bif
	// implementation.

	symbol = ::util::strreplace(symbol, "::", "_");

	auto ns = GlueModuleBuilder()->module()->id()->name();
	auto qualified_symbol = ::util::fmt("%s::%s", ns, symbol);

	if ( declared(symbol) )
		// We already did all the work earlier.
		return ::hilti::builder::id::create(symbol);

	auto glue_mbuilder = Compiler()->GlueModuleBuilder();

	if ( ! glue_mbuilder->declared(symbol) )
		{
		Compiler()->pushModuleBuilder(glue_mbuilder);

		auto stub = glue_mbuilder->pushFunction(symbol, ftype);
		glue_mbuilder->exportID(stub->id());

		RecordType* args = bif->FType()->Args();

		::hilti::builder::tuple::element_list func_args;

		for ( int i = 0; i < args->NumFields(); i++ )
			{
			auto arg = ::hilti::builder::id::create(args->FieldName(i));
			auto btype = args->FieldType(i);
			auto val = RuntimeHiltiToVal(arg, btype);
			func_args.push_back(val);
			}

		auto f = ::hilti::builder::bytes::create(bif->Name());
		auto t = ::hilti::builder::tuple::create(func_args);
		auto ca = ::hilti::builder::tuple::create({ f, t });

		auto ytype = bif->FType()->YieldType();

		if ( ytype && ytype->Tag() != TYPE_VOID && ytype->Tag() != TYPE_ANY )
			{
			auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");
			auto rval = glue_mbuilder->addTmp("rval", vtype);

			Builder()->addInstruction(rval, ::hilti::instruction::flow::CallResult,
						  ::hilti::builder::id::create("LibBro::call_bif_result"), ca);

			auto result = RuntimeValToHilti(rval, ytype);
			Builder()->addInstruction(::hilti::instruction::flow::ReturnResult, result);
			}

		else
			Builder()->addInstruction(::hilti::instruction::flow::CallVoid,
						  ::hilti::builder::id::create("LibBro::call_bif_void"), ca);

		glue_mbuilder->popFunction();

		Compiler()->popModuleBuilder();
		}

	declareFunction(qualified_symbol, ftype);
	return ::hilti::builder::id::create(qualified_symbol);
	}

