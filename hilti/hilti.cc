
#include "hilti-intern.h"
#include "autogen/config.h"

// Auto-generated in ${autogen}/instructions-register.cc
extern void __registerAllInstructions();

static const path_list _prepareLibDirs(const path_list& libdirs)
{
    // Always add libhilti to the path list.
    auto paths = libdirs;
    paths.push_front(hilti::CONFIG_PATH_LIBHILTI);
    return paths;
}

void hilti::init()
{
    __registerAllInstructions();
}

extern string hilti::version()
{
    return CONFIG_HILTI_VERSION;
}

shared_ptr<Module> hilti::parseStream(std::istream& in, const std::string& sname)
{
    hilti_parser::Driver driver;
    return driver.parse(in, sname);
}

bool hilti::printAST(shared_ptr<Module> module, std::ostream& out)
{
    passes::Printer printer(out);
    return printer.run(module);
}

llvm::Module* hilti::compileModule(shared_ptr<Module> module, const path_list& libdirs, bool verify, int debug, int profile, int debug_cg)
{
    codegen::CodeGen cg(_prepareLibDirs(libdirs), debug_cg);
    return cg.generateLLVM(module, verify, debug, profile);
}

bool hilti::dumpAST(shared_ptr<Node> ast, std::ostream& out)
{
    ast->dump(out);
    return true;
}

shared_ptr<Module> hilti::loadModule(shared_ptr<ID> id, const path_list& libdirs, bool import)
{
    return loadModule(id->name(), libdirs, import);
}

bool hilti::validateAST(shared_ptr<Module> module)
{
    passes::Validator validator;
    return validator.run(module);
}

void hilti::printAssembly(llvm::raw_ostream &out, llvm::Module* module)
{
    hilti::codegen::AssemblyAnnotationWriter annotator;
    module->print(out, &annotator);
}

void hilti::generatePrototypes(shared_ptr<Module> module, std::ostream& out)
{
    codegen::ProtoGen pg(out);
    return pg.generatePrototypes(module);
}

llvm::Module* hilti::linkModules(string output, const std::list<llvm::Module*>& modules, const path_list& paths, const std::list<string>& libs, const path_list& bcas, bool verify, int debug_cg)
{
    codegen::Linker linker(libs);

    for ( auto l : libs )
        linker.addNativeLibrary(l);

    for ( auto b : bcas )
        linker.addBitcodeArchive(b);

    return linker.link(output, modules, verify, debug_cg);
}

bool hilti::resolveAST(shared_ptr<Module> module, const path_list& libdirs)
{
//    std::cerr << "| 11111" << std::endl;
//    module->dump(std::cerr);

    passes::ScopeBuilder scope_builder(_prepareLibDirs(libdirs));
    if ( ! scope_builder.run(module) )
        return false;

#if 0
    passes::ScopePrinter scope_printer;
    if ( ! scope_printer.run(module) )
        return false;
#endif

//    std::cerr << "| 2222" << std::endl;
//    module->dump(std::cerr);

    passes::IdResolver id_resolver;
    if ( ! id_resolver.run(module) )
        return false;

//    std::cerr << "| 3333" << std::endl;
//    module->dump(std::cerr);


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

#if 0
    if ( ! hilti::printAST(module, std::cerr) )
        return false;
#endif

    return true;
}

shared_ptr<Module> hilti::loadModule(string path, const path_list& libdirs, bool import)
{
    if ( ! util::endsWith(path, ".hlt") )
        path += ".hlt";

    path = util::strtolower(path);

    auto dirs = _prepareLibDirs(libdirs);

    string full_path = util::findInPaths(path, dirs);

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

    if ( ! resolveAST(module, dirs) )
        return nullptr;

    if ( ! validateAST(module) )
        return nullptr;

    return module;
}

instruction_list hilti::instructions()
{
    instruction_list instrs;

    for ( auto i : InstructionRegistry::globalRegistry()->getAll() )
        instrs.push_back(i.second);

    return instrs;
}
