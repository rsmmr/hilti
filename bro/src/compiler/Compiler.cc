
#include <dlfcn.h>

#include <Traverse.h>
#include <Scope.h>
#include <Dict.h>
#include <Hash.h>
#include <Func.h>
#include <Obj.h>
#undef List

#include <util/util.h>

#include "Compiler.h"
#include "ModuleBuilder.h"
#include "../LocalReporter.h"

namespace BifConst { namespace Hilti {  extern int save_hilti;  }  }

using namespace bro::hilti::compiler;

// Walk the Bro AST to normalize nodes in preparation for compiling into
// HILTI.
class NormalizerCallBack : public ::TraversalCallback {
public:
	NormalizerCallBack(Compiler* compiler) : compiler(compiler) {};
	TraversalCode PreID(const ::ID* id);
private:
	Compiler* compiler;
};

TraversalCode NormalizerCallBack::PreID(const ::ID* id)
	{
	if ( (! id->ID_Val()) && id->Type()->Tag() == TYPE_FUNC && compiler->HaveHiltiBif(id->Name()) )
		{
		// A HILTI-only built-in function won't have a value. Fake one.
		auto non_const_id = const_cast<::ID *>(id);
		auto fval = new ::Val(new ::BuiltinFunc(0, id->Name(), true));
		non_const_id->SetVal(fval);
		}

	return TC_CONTINUE;
	}

Compiler::Compiler(shared_ptr<::hilti::CompilerContext> arg_hilti_context)
	{
	hilti_context = arg_hilti_context;

	auto glue_id = ::hilti::builder::id::node("BroGlue");
	glue_module_builder = std::make_shared<::hilti::builder::ModuleBuilder>(hilti_context, glue_id);
	}

Compiler::~Compiler()
	{
	}

std::shared_ptr<::hilti::builder::BlockBuilder> Compiler::Builder() const
	{
	return mbuilders.back()->builder();
	}

::hilti::builder::ModuleBuilder* Compiler::ModuleBuilder()
	{
	return mbuilders.back();
	}

void Compiler::pushModuleBuilder(::hilti::builder::ModuleBuilder* mbuilder)
	{
	mbuilders.push_back(mbuilder);
	}

void Compiler::popModuleBuilder()
	{
	assert(mbuilders.size() > 0);
	mbuilders.pop_back();
	}

Compiler::module_list Compiler::CompileAll()
	{
	NormalizerCallBack cb(this);
	traverse_all(&cb);

	Compiler::module_list modules;

	for ( auto ns : GetNamespaces() )
		{
		auto mbuilder = new class ModuleBuilder(this, hilti_context, ns);

		pushModuleBuilder(mbuilder);

        try {
            auto module = mbuilder->Compile();

            if ( ! module )
                return Compiler::module_list();

            modules.push_back(module);
        }

        catch ( std::exception& e ) {
            std::cerr << "Uncaught exception: " << e.what() << std::endl;
            abort();
        }

		popModuleBuilder();

		delete mbuilder;
		}

	return modules;
	}

std::shared_ptr<::hilti::Module> Compiler::FinalizeGlueBuilder()
	{
	if ( BifConst::Hilti::save_hilti )
		glue_module_builder->saveHiltiCode(::util::fmt("bro.%s.hlt", glue_module_builder->module()->id()->name()));

	glue_module_builder->importModule("Hilti");
	glue_module_builder->importModule("LibBro");

	return glue_module_builder->finalize();
	}

::hilti::builder::ModuleBuilder* Compiler::GlueModuleBuilder() const
	{
	return glue_module_builder.get();
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
	return normalizeSymbol(func->Name(), "", "", module ? module->id()->name() : "", true, include_module);
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
	return normalizeSymbol(func->Name(), "", "stub", module ? module->id()->name() : "", true, include_module);

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
	return normalizeSymbol(id->Name(), "", "", module ? module->id()->name() : "", id->IsGlobal());
	}

string Compiler::HiltiSymbol(const std::string& id, bool global, const std::string& module, bool include_module)
	{
	return normalizeSymbol(id, "", "", module, global, include_module);
	}

std::string Compiler::normalizeSymbol(const std::string sym, const std::string prefix, const std::string postfix, const std::string& module, bool global, bool include_module)
	{
	auto id = ::hilti::builder::id::node(sym);

	auto local = id->local();

	if ( prefix.size() )
		local = ::util::fmt("%s_%s", prefix, local);

	if ( postfix.size() )
		local = ::util::fmt("%s_%s", local, postfix);

	if ( module.size() && ::util::strtolower(id->scope()) == ::util::strtolower(module) )
		return local;

	if ( global && id->scope().size() && ! include_module )
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

std::string Compiler::HiltiODescSymbol(const ::BroObj* obj)
	{
	::ODesc d;
	obj->Describe(&d);

	std::string sym = d.Description();

	for ( auto p = sym.begin(); p != sym.end(); ++p )
		{
		if ( ! isalnum(*p) )
			*p = '_';
		}

	while ( sym.find("__") != std::string::npos )
		sym = ::util::strreplace(sym, "__", "_");

	return sym;
	}

bool Compiler::HaveHiltiBif(const std::string& name, std::string* hilti_name)
	{
	auto bif_symbol = ::util::fmt("%s_bif", HiltiSymbol(name, true, "", true));

	// See if we have a statically compiled bif function available.
	if ( ! dlsym(RTLD_DEFAULT, bif_symbol.c_str()) )
		return false;

	if ( hilti_name )
		*hilti_name = name + "_bif";

	return true;
	}
