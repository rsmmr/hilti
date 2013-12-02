
// FIXME: Due to namespace issues, we can't have this in context.cc.

#include "binpac/autogen/binpac-config.h"

#include "context.h"
#include "options.h"

#include <hilti/context.h>
#include <hilti/jit/libhilti-jit.h>

binpac::CompilerContext::CompilerContext(const Options& options)
{
    setOptions(options);
}

void binpac::CompilerContext::setOptions(const Options& options)
{
    _options = std::shared_ptr<Options>(new Options(options));
    _hilti_context = std::make_shared<hilti::CompilerContext>(options);

    if ( options.cgDebugging("visitors") )
        ast::enableDebuggingForAllVisitors();
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<llvm::Module*> modules, std::list<string> libs, path_list bcas, path_list dylds, bool add_stdlibs, bool add_sharedlibs)
{
    if ( add_stdlibs ) {
        if ( options().debug )
            bcas.push_back(configuration().runtime_library_bca_dbg);
        else
            bcas.push_back(configuration().runtime_library_bca);
    };

    return _hilti_context->linkModules(output, modules, libs, bcas, dylds, add_stdlibs, add_sharedlibs);
}

llvm::Module* binpac::CompilerContext::linkModules(string output, std::list<llvm::Module*> modules)
{
    return linkModules(output, modules,
                       std::list<string>(),
                       path_list(), path_list());
}
