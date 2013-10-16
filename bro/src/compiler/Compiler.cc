
#include <Traverse.h>
#include <Scope.h>
#include <Dict.h>
#include <Hash.h>
#include <Func.h>
#undef List

#include <util/util.h>

#include "Compiler.h"
#include "ModuleBuilder.h"
#include "../LocalReporter.h"

using namespace bro::hilti::compiler;

Compiler::Compiler(shared_ptr<::hilti::CompilerContext> arg_hilti_context)
	{
	hilti_context = arg_hilti_context;
	}

Compiler::~Compiler()
	{
	}

Compiler::module_list Compiler::CompileAll()
	{
	Compiler::module_list modules;

	for ( auto ns : GetNamespaces() )
		{
		auto mbuilder = new ModuleBuilder(this, hilti_context, ns);
		auto module = mbuilder->Compile();

		if ( ! module )
			return Compiler::module_list();

		modules.push_back(module);

		delete mbuilder;
		}

	return modules;
	}

std::list<string> Compiler::GetNamespaces() const
	{
	auto ids = global_scope()->GetIDs();

	std::set<string> nsset;

	IterCookie* iter = ids->InitForIteration();

	while ( auto id = ids->NextEntry(iter) )
		nsset.insert(id->ModuleName());

	std::list<string> namespaces;

	for ( auto ns : nsset )
		namespaces.push_back(ns);

	namespaces.sort();
	return namespaces;
	}

string Compiler::HiltiSymbol(const ::Func* func, shared_ptr<::hilti::Module> module, bool include_module)
	{
	return normalizeSymbol(func->Name(), "", "", module, include_module);
#if 0
	if ( func->GetKind() == Func::BUILTIN_FUNC )
		return func->Name();

	switch ( func->FType()->Flavor() ) {
	case FUNC_FLAVOR_FUNCTION:
		name = ::util::fmt("bro_function_%s", func->Name());
		break;

	case FUNC_FLAVOR_EVENT:
		name = ::util::fmt("bro_event_%s", func->Name());
		break;

	case FUNC_FLAVOR_HOOK:
		name = ::util::fmt("bro_hook_%s", func->Name());
		break;

	default:
		reporter::internal_error("unknown function flavor");
	}

	return ::util::strreplace(name, "::", "_");
#endif
	}

std::string Compiler::HiltiStubSymbol(const ::Func* func, shared_ptr<::hilti::Module> module, bool include_module)
	{
	return normalizeSymbol(func->Name(), "", "stub", module, include_module);

#if 0
	auto name = func->Name();

	auto var = extract_var_name(name);
	auto mod = extract_module_name(name);

	if ( mod.empty() )
		mod = "GLOBAL";

	mod = ::util::strtolower(mod);

	if ( include_module )
		return ::util::fmt("%s_bro_event_%s_stub", mod, name);
	else
		return ::util::fmt("bro_event_%s_stub", var);
#endif
	}

string Compiler::HiltiSymbol(const ::ID* id, shared_ptr<::hilti::Module> module)
	{
	return normalizeSymbol(id->Name(), "", "", module);
	}

std::string Compiler::normalizeSymbol(const std::string sym, const std::string prefix, const std::string postfix, shared_ptr<::hilti::Module> module, bool include_module)
	{
	auto id = ::hilti::builder::id::node(sym);

	auto local = id->local();

	if ( prefix.size() )
		local = ::util::fmt("%s_%s", prefix, local);

	if ( postfix.size() )
		local = ::util::fmt("%s_%s", local, postfix);

	if ( module && ::util::strtolower(id->scope()) == ::util::strtolower(module->id()->name()) )
		return local;

	if ( id->scope().size() && ! include_module )
		return ::util::fmt("%s::%s", id->scope(), local);

	if ( include_module )
		{
		auto scope = id->scope();

		if ( scope.empty() )
			scope = "GLOBAL";

		return ::util::fmt("%s_%s", ::util::strtolower(scope), local);
		}

	return local;
	}

