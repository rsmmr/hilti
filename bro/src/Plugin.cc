

#include "Plugin.h"
#include "Pac2Analyzer.h"
#include "Manager.h"
#include "LocalReporter.h"
#include "analyzer/Component.h"

plugin::Bro_Hilti::Plugin HiltiPlugin;

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

void plugin::Bro_Hilti::Plugin::InitPreScript()
	{
	plugin::Plugin::InitPreScript();

	SetName("Bro::Hilti");
	SetDynamicPlugin(true);
	SetFileExtensions("pac2:evt");

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

void plugin::Bro_Hilti::Plugin::InitPostScript()
	{
	plugin::Plugin::InitPostScript();

	if ( ! _manager->InitPostScripts() )
		exit(1);

	if ( ! _manager->FinishLoading() )
		exit(1);

	if ( ! _manager->Compile() )
		exit(1);
	}

void plugin::Bro_Hilti::Plugin::Done()
	{
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

void plugin::Bro_Hilti::Plugin::AddEvent(const string& name)
	{
	AddBifItem(name.c_str(), plugin::BifItem::EVENT);
	}
