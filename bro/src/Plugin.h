
#ifndef BRO_PLUGIN_HILTI_PLUGIN_H
#define BRO_PLUGIN_HILTI_PLUGIN_H

#include <plugin/Plugin.h>
#include <analyzer/Tag.h>
#include <file_analysis/Tag.h>
#include <net_util.h>

namespace bro { namespace hilti  {

struct Pac2AnalyzerInfo;
class Manager;

} }

namespace plugin { namespace Bro_Hilti {

class Plugin : public plugin::Plugin {
public:
	Plugin();
	virtual ~Plugin();

	bro::hilti::Manager* Mgr() const;

	analyzer::Tag AddAnalyzer(const std::string& name, TransportProto proto, analyzer::Tag::subtype_t stype);
	file_analysis::Tag AddFileAnalyzer(const std::string& name, file_analysis::Tag::subtype_t stype);

	void AddEvent(const std::string& name);

protected:
	// Overridden from Bro's Plugin.
	void InitPreScript() override;
	void InitPostScript() override;
	bool LoadFile(const char* file) override;
	void Done() override;

private:
	bro::hilti::Manager* _manager;
	plugin::Plugin::component_list components;
};

} }

extern plugin::Bro_Hilti::Plugin HiltiPlugin;

#endif
