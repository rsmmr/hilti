
#include "Conn.h"
#undef List

#include "Plugin.h"
#include "Pac2Analyzer.h"
#include "Pac2FileAnalyzer.h"
#include "Manager.h"
#include "LocalReporter.h"
#include "analyzer/Component.h"
#include "Event.h"
#include "RuntimeInterface.h"

#ifdef BRO_PLUGIN_CHECK_LEAKS
#include <google/heap-checker.h>
static HeapLeakChecker* heap_checker = nullptr;
#endif

#ifdef BRO_PLUGIN_HAVE_PROFILING
extern int time_bro;
#endif

plugin::Bro_Hilti::Plugin HiltiPlugin;

namespace BifConst { namespace Hilti { extern int compile_scripts; } }

using namespace bro::hilti;

plugin::Bro_Hilti::Plugin::Plugin()
	{
	_manager = new bro::hilti::Manager();
	}

plugin::Bro_Hilti::Plugin::~Plugin()
	{
	delete _manager;
	}

bro::hilti::Manager* plugin::Bro_Hilti::Plugin::Mgr() const
	{
	return _manager;
	}

plugin::Configuration plugin::Bro_Hilti::Plugin::Configure()
	{
	plugin::Configuration config;
	config.name = "Bro::Hilti";
	config.description = "Dynamically compiled HILTI/BinPAC++ functionality (*.pac2, *.evt, *.hlt)";
	config.version.major = 0;
	config.version.minor = 1;

	EnableHook(plugin::HOOK_LOAD_FILE);

	return config;
	}

void plugin::Bro_Hilti::Plugin::InitPreScript()
	{
	string dir = HiltiPlugin.PluginDirectory();
	dir += "/pac2";
	_manager->AddLibraryPath(dir.c_str());

	if ( ! _manager->InitPreScripts() )
		exit(1);
	}

void plugin::Bro_Hilti::Plugin::InitPostScript()
	{
	plugin::Plugin::InitPostScript();

	if ( BifConst::Hilti::compile_scripts || _manager->HaveCustomHiltiCode() )
		{
		EnableHook(plugin::HOOK_CALL_FUNCTION);
		EnableHook(plugin::HOOK_QUEUE_EVENT);
		EnableHook(plugin::HOOK_UPDATE_NETWORK_TIME);
		EnableHook(plugin::HOOK_DRAIN_EVENTS);
		EnableHook(plugin::HOOK_BRO_OBJ_DTOR);
		}

	if ( ! _manager->InitPostScripts() )
		exit(1);

	if ( ! _manager->FinishLoading() )
		exit(1);

	if ( ! _manager->Compile() )
		exit(1);

#ifdef BRO_PLUGIN_CHECK_LEAKS
	if ( getenv("HEAPCHECK") )
	     heap_checker = new HeapLeakChecker("bro-hilti");
#endif

#ifdef BRO_PLUGIN_HAVE_PROFILING
	if ( time_bro )
		fprintf(stderr, "#! Processing starting ...\n");
#endif
	}

void plugin::Bro_Hilti::Plugin::Done()
	{
#ifdef BRO_PLUGIN_CHECK_LEAKS
	if ( heap_checker )
		{
		fprintf(stderr, "#! Done with leak checking\n");

		if ( ! heap_checker->NoLeaks() )
			fprintf(stderr, "#! Leaks found\n");
		else
			fprintf(stderr, "#! Leaks NOT found\n");

		delete heap_checker;
		}
#endif
	}

analyzer::Tag plugin::Bro_Hilti::Plugin::AddAnalyzer(const string& name, TransportProto proto, analyzer::Tag::subtype_t stype)
	{
	analyzer::Component::factory_callback factory = 0;

	switch ( proto ) {
	case TRANSPORT_TCP:
		factory = Pac2_TCP_Analyzer::InstantiateAnalyzer;
		break;

	case TRANSPORT_UDP:
		factory = Pac2_UDP_Analyzer::InstantiateAnalyzer;
		break;

	default:
		reporter::error("unsupported protocol in analyzer");
		return analyzer::Tag();
	}

	auto c = new ::analyzer::Component(name, factory, stype);
	AddComponent(c);

	return c->Tag();
	}

file_analysis::Tag plugin::Bro_Hilti::Plugin::AddFileAnalyzer(const string& name, file_analysis::Tag::subtype_t stype)
	{
	auto c = new ::file_analysis::Component(name, Pac2_FileAnalyzer::InstantiateAnalyzer, stype);
	AddComponent(c);
	return c->Tag();
	}

void plugin::Bro_Hilti::Plugin::AddEvent(const string& name)
	{
	AddBifItem(name, plugin::BifItem::EVENT);
	}

void plugin::Bro_Hilti::Plugin::RequestEvent(EventHandlerPtr handler)
	{
	plugin::Plugin::RequestEvent(handler);
	}

int plugin::Bro_Hilti::Plugin::HookLoadFile(const std::string& file, const std::string& ext)
	{
	if ( ext == "pac2" || ext == "evt" || ext == "hlt" )
		return _manager->LoadFile(file) ? 1 : 0;

	return -1;
	}

std::pair<bool, Val*> plugin::Bro_Hilti::Plugin::HookCallFunction(const Func* func, Frame* parent, val_list* args)
	{
	if ( func->GetKind() == BuiltinFunc::BUILTIN_FUNC )
		return std::pair<bool, Val*>(false, nullptr);

	if ( BifConst::Hilti::compile_scripts )
		return _manager->RuntimeCallFunction(func, parent, args);

	if ( func->FType()->Flavor() == FUNC_FLAVOR_EVENT &&
	     _manager->HaveCustomHandler(func) )
		{
		// We don't own the arguments so ref them all once more.
		loop_over_list(*args, i)
			Ref((*args)[i]);

		auto v = _manager->RuntimeCallFunction(func, parent, args);
		Unref(v.second);
		}

	return std::pair<bool, Val*>(false, nullptr);
	}

bool plugin::Bro_Hilti::Plugin::HookQueueEvent(Event* event)
	{
	if ( ! BifConst::Hilti::compile_scripts )
		return false;

	if ( ! _manager->RuntimeRaiseEvent(event) )
		{
		val_list* args = event->Args();

		loop_over_list(*args, i)
			Unref((*args)[i]);

		Unref(event);
		}

	// We always say we handled it.
	return true;
	}

void plugin::Bro_Hilti::Plugin::HookUpdateNetworkTime(double network_time)
	{
	}

void plugin::Bro_Hilti::Plugin::HookDrainEvents()
	{
	}

void plugin::Bro_Hilti::Plugin::HookBroObjDtor(void* obj)
	{
	lib_bro_object_mapping_unregister_bro(obj);
	}
