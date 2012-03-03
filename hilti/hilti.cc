
#include "hilti.h"

bool hilti::resolveAST(shared_ptr<Module> module, const path_list& libdirs)
{
    passes::ScopeBuilder scope_builder(libdirs);
    if ( ! scope_builder.run(module) )
        return false;

#if 0
    passes::ScopePrinter scope_printer;
    if ( ! scope_printer.run(module) )
        return false;
#endif

    passes::IdResolver id_resolver;
    if ( ! id_resolver.run(module) )
        return false;

#if 0
    if ( ! hilti::dumpAST(module, std::cerr) )
        return false;
#endif

#if 0
    if ( ! hilti::printAST(module, std::cerr) )
        return false;
#endif

    passes::InstructionResolver instruction_resolver;
    if ( ! instruction_resolver.run(module) )
        return false;

    passes::BlockNormalizer block_normalizer;
    if ( ! block_normalizer.run(module) )
        return false;

    // Run these again, we have inserted new instructions.

    if ( ! id_resolver.run(module) )
        return false;

    if ( ! instruction_resolver.run(module) )
        return false;

    return true;
}
