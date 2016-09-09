
#ifndef BRO_PLUGIN_HILTI_PLUGIN_H
#define BRO_PLUGIN_HILTI_PLUGIN_H

#include <analyzer/Tag.h>
#include <file_analysis/Tag.h>
#include <net_util.h>
#include <plugin/Plugin.h>

// #define BRO_PLUGIN_CHECK_LEAKS
// #define BRO_PLUGIN_HAVE_PROFILING

namespace bro {
namespace hilti {

struct Pac2AnalyzerInfo;
class Manager;
}
}

namespace plugin {
namespace Bro_Hilti {

class Plugin : public ::plugin::Plugin {
public:
    Plugin();
    virtual ~Plugin();

    bro::hilti::Manager* Mgr() const;

    analyzer::Tag AddAnalyzer(const std::string& name, TransportProto proto,
                              analyzer::Tag::subtype_t stype);
    file_analysis::Tag AddFileAnalyzer(const std::string& name,
                                       file_analysis::Tag::subtype_t stype);

    void AddEvent(const std::string& name);

    // Forwards to Plugin::RequestEvents(), which is protectd.
    void RequestEvent(EventHandlerPtr handler);

protected:
    // Overriden methods from Bro's plugin API.

    plugin::Configuration Configure() override;
    void InitPostScript() override;
    void InitPreScript() override;
    void Done() override;

    // We always use this hook.
    int HookLoadFile(const std::string& file, const std::string& ext) override;

    // We activate these hooks only when compiling scripts or injecting
    // custom HILTI code.
    std::pair<bool, Val*> HookCallFunction(const Func* func, Frame* parent,
                                           val_list* args) override;
    bool HookQueueEvent(Event* event) override;
    void HookUpdateNetworkTime(double network_time) override;
    void HookDrainEvents() override;
    void HookBroObjDtor(void* obj) override;

private:
    bro::hilti::Manager* _manager;
    plugin::Plugin::component_list components;
};
}
}

extern plugin::Bro_Hilti::Plugin HiltiPlugin;

#endif
