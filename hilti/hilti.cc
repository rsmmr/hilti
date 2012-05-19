
#include "hilti.h"

// Auto-generated in ${autogen}/instructions-register.cc
extern void __registerAllInstructions();

void hilti::init()
{
    __registerAllInstructions();
}

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

shared_ptr<Module> loadModule(string path, const path_list& libdirs)
{
    if ( ! util::endsWith(path, ".hlt") )
        path += ".hlt";

    path = util::strtolower(path);

    string full_path = util::findInPaths(path, libdirs);

    if ( full_path.size() == 0 ) {
        std::cerr << "cannot find " + path << std::endl;
        return nullptr;
    }

    shared_ptr<Module> module = Module::getExistingModule(full_path);

    if ( module )
        return module;

    std::ifstream in(full_path);

    if ( in.fail() ) {
        std::cerr << "cannot open " + full_path << " for reading" << std::endl;
        return nullptr;
    }

    module = hilti::parseStream(in, full_path);

    if ( ! module )
        return nullptr;

    if ( ! resolveAST(module, libdirs) )
        return nullptr;

    if ( ! validateAST(module) )
        return nullptr;

    return module;
}

shared_ptr<Module> hilti::loadModule(string path, const path_list& libdirs, bool import)
{
    if ( ! util::endsWith(path, ".hlt") )
        path += ".hlt";

    path = util::strtolower(path);

    string full_path = util::findInPaths(path, libdirs);

    if ( full_path.size() == 0 ) {
        std::cerr << "cannot find " + path << std::endl;
        return nullptr;
    }

    shared_ptr<Module> module = Module::getExistingModule(full_path);

    if ( module )
        return module;

    std::ifstream in(full_path);

    if ( in.fail() ) {
        std::cerr << "cannot open " + full_path << " for reading" << std::endl;
        return nullptr;
    }

    module = hilti::parseStream(in, full_path);

    if ( ! module )
        return nullptr;

    passes::ScopeBuilder scope_builder(libdirs);

    if ( ! scope_builder.run(module) )
        return nullptr;

    if ( ! resolveAST(module, libdirs) )
        return nullptr;

    if ( ! validateAST(module) )
        return nullptr;

    return module;
}
