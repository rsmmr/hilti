
// FIXME: Due to namespace issues, we can't have this in context.cc.

#include "autogen/binpac-config.h"

#include "context.h"
#include "options.h"
#include "hilti/context.h"
#include "jit/libhilti-jit.h"

binpac::CompilerContext::CompilerContext(const Options& options)
{
    _options = std::shared_ptr<Options>(new Options(options));
    _hilti_context = std::make_shared<hilti::CompilerContext>(options);

    if ( options.cgDebugging("visitors") )
        ast::enableDebuggingForAllVisitors();
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<shared_ptr<hilti::Module>> modules, std::list<string> libs, path_list bcas, path_list dylds, bool add_stdlibs, bool add_sharedlibs)
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
        auto llvm_module = _hilti_context->compile(m);

        if ( ! llvm_module )
            return nullptr;

        llvm_modules.push_back(llvm_module);
    }

    if ( add_stdlibs ) {
        if ( options().debug )
            bcas.push_back(configuration().runtime_library_bca_dbg);
        else
            bcas.push_back(configuration().runtime_library_bca);
    };

    return _hilti_context->linkModules(output, llvm_modules, libs, bcas, dylds, add_stdlibs, add_sharedlibs);
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<shared_ptr<hilti::Module>> modules)
{
    return linkModules(output, modules,
                       std::list<string>(),
                       path_list(), path_list());
}
