
// FIXME: Due to namespace issues, we can't have this in context.cc.

#include "autogen/binpac-config.h"

#include "context.h"
#include "hilti/context.h"
#include "jit/libhilti-jit.h"

binpac::CompilerContext::CompilerContext(const string_list& libdirs)
{
    auto paths = libdirs;

    for ( auto p : configuration().binpac_library_dirs )
        paths.push_back(p);

    _libdirs = paths;
    _hilti_context = std::make_shared<hilti::CompilerContext>(paths);
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<shared_ptr<hilti::Module>> modules, path_list paths, std::list<string> libs, path_list bcas, path_list dylds, bool debug, bool verify, bool profile, bool add_stdlibs, bool add_sharedlibs)
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
            bcas.push_back(configuration().runtime_library_bca_dbg);
        else
            bcas.push_back(configuration().runtime_library_bca);
    };

    return _hilti_context->linkModules(output, llvm_modules, paths, libs, bcas, dylds, debug, verify, add_stdlibs, add_sharedlibs);
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<shared_ptr<hilti::Module>> modules,
                                           bool debug, bool profile)
{
    return linkModules(output, modules,
                       path_list(), std::list<string>(),
                       path_list(), path_list(),
                       debug, true, profile);
}

bool binpac::CompilerContext::enableDebug(const string& label)
{
    _hilti_context->enableDebug(label);

    if ( ! validDebugStream(label) )
        return false;

    _debug_streams.insert(label);
    return true;
}
