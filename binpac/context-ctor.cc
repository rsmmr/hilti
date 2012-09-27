
// FIXME: Due to namespace issues, we can't have this in context.cc.

#include "autogen/config.h"

#include "context.h"
#include "hilti/context.h"

binpac::CompilerContext::CompilerContext(const string_list& libdirs)
{
    // Always add libbinpac to the path list.
    auto paths = libdirs;
    paths.push_front(".");

    for ( auto p : configuration().library_dirs )
        paths.push_back(p);

    _libdirs = paths;
    _hilti_context = std::make_shared<hilti::CompilerContext>(paths);
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<shared_ptr<hilti::Module>> modules, path_list paths, std::list<string> libs, path_list bcas, path_list dylds, bool debug, bool verify, bool profile, bool add_stdlibs)
{
#if 0
    if ( add_stdlibs ) {
        auto runtime_hlt = _hilti_context->loadModule(configuration().runtime_hlt);

        if ( ! runtime_hlt )
            return nullptr;

        modules.push_back(runtime_hlt);
    }
#endif    

    std::list<llvm::Module*> llvm_modules;

    for ( auto m : modules ) {
        auto llvm_module = _hilti_context->compile(m, debug, verify, profile);

        if ( ! llvm_module )
            return nullptr;

        llvm_modules.push_back(llvm_module);
    }

    if ( add_stdlibs ) {
        if ( debug )
            bcas.push_back(configuration().runtime_dbg_bca);
        else
            bcas.push_back(configuration().runtime_bca);
    };

    return _hilti_context->linkModules(output, llvm_modules, paths, libs, bcas, dylds, debug, verify, add_stdlibs);
}

binpac::CompilerContext::binpac_parsers_func binpac::CompilerContext::jitBinpacParser(llvm::Module* module, int optimize)
{
    auto ee = _hilti_context->jitModule(module, optimize);

    if ( ! ee )
        return nullptr;

    auto func = _hilti_context->nativeFunction(ee, module, "binpac_parsers");

    if ( ! func )
        return nullptr;

    return (binpac_parsers_func)func;
}

bool binpac::CompilerContext::enableDebug(const string& label)
{
    _hilti_context->enableDebug(label);

    if ( ! validDebugStream(label) )
        return false;

    _debug_streams.insert(label);
    return true;
}
