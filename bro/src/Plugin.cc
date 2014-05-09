
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

plugin::Bro_Hilti::Plugin HiltiPlugin;

namespace BifConst { namespace Hilti { extern int compile_scripts; } }

using namespace bro::hilti;

plugin::Bro_Hilti::Plugin::Plugin()
	: ::plugin::InterpreterPlugin(100)
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

void plugin::Bro_Hilti::Plugin::InitPreScript()
	{
	SetName("Bro::Hilti");
	SetDynamicPlugin(true);
	SetFileExtensions("pac2:evt:hlt");

	BRO_PLUGIN_VERSION(1);
	BRO_PLUGIN_DESCRIPTION("Dynamically compiled HILTI/BinPAC++ functionality");
	BRO_PLUGIN_BIF_FILE(consts);
	BRO_PLUGIN_BIF_FILE(events);
	BRO_PLUGIN_BIF_FILE(functions);

	string dir = HiltiPlugin.PluginDirectory();
	dir += "/pac2";
	_manager->AddLibraryPath(dir.c_str());

	if ( ! _manager->InitPreScripts() )
		exit(1);
	}

#if 0
#include <google/heap-checker.h>

static HeapLeakChecker* heap_checker = nullptr;
#endif

extern int time_bro;

void plugin::Bro_Hilti::Plugin::InitPostScript()
	{
	plugin::Plugin::InitPostScript();

	if ( ! _manager->InitPostScripts() )
		exit(1);

	if ( ! _manager->FinishLoading() )
		exit(1);

	if ( ! _manager->Compile() )
		exit(1);

#if 0
	if ( getenv("HEAPCHECK") )
	     heap_checker = new HeapLeakChecker("bro-hilti");
#endif

	if ( time_bro )
		fprintf(stderr, "#! Processing starting ...\n");
	}

void plugin::Bro_Hilti::Plugin::Done()
	{
#if 0
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

bool plugin::Bro_Hilti::Plugin::LoadFile(const char* file)
	{
	return _manager->LoadFile(file);
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

	auto c = new analyzer::Component(name.c_str(), factory, stype);

#if 0
	components.push_back(c);
#endif

	AddComponent(c);
	return c->Tag();
	}

file_analysis::Tag plugin::Bro_Hilti::Plugin::AddFileAnalyzer(const string& name, file_analysis::Tag::subtype_t stype)
	{
	auto c = new file_analysis::Component(name.c_str(), Pac2_FileAnalyzer::InstantiateAnalyzer, stype);
	AddComponent(c);
	return c->Tag();
	}

void plugin::Bro_Hilti::Plugin::AddEvent(const string& name)
	{
	AddBifItem(name.c_str(), plugin::BifItem::EVENT);
	}

Val* plugin::Bro_Hilti::Plugin::CallFunction(const Func* func, val_list* args)
	{
	if ( BifConst::Hilti::compile_scripts )
		return _manager->RuntimeCallFunction(func, args);

	if ( func->FType()->Flavor() == FUNC_FLAVOR_EVENT &&
	     _manager->HaveCustomHandler(func) )
		{
		// We don't own the arguments so ref them all once more.
		loop_over_list(*args, i)
			Ref((*args)[i]);

		auto v = _manager->RuntimeCallFunction(func, args);
		Unref(v);
		}

	return nullptr;
	}

bool plugin::Bro_Hilti::Plugin::QueueEvent(Event* event)
	{
	if ( ! BifConst::Hilti::compile_scripts )
		return false;

	if ( ! _manager->RuntimeRaiseEvent(event) )
		Unref(event);

	// We always say we handled it.
	return true;
	}

void plugin::Bro_Hilti::Plugin::UpdateNetworkTime(double network_time)
	{
	}

void plugin::Bro_Hilti::Plugin::DrainEvents()
	{
	}

void plugin::Bro_Hilti::Plugin::NewConnection(const ::Connection* c)
	{
	}

void plugin::Bro_Hilti::Plugin::ConnectionStateRemove(const ::Connection* c)
	{
	}

void plugin::Bro_Hilti::Plugin::BroObjDtor(const BroObj* obj)
	{
	lib_bro_object_mapping_unregister_bro(obj);
	}
