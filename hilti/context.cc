
#include <fstream>
#include <util/util.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Target/TargetOptions.h>

#include "hilti-intern.h"
#include "autogen/hilti-config.h"

using namespace hilti;
using namespace hilti::passes;

CompilerContext::CompilerContext(const string_list& libdirs)
{
    string_list paths = libdirs;

    for ( auto p : hilti::configuration().hilti_library_dirs )
        paths.push_back(p);

    paths.push_front(".");  // Always add cwd at the front.

    _libdirs = paths;
}

string CompilerContext::searchModule(shared_ptr<ID> id)
{
    auto p = util::strtolower(id->pathAsString());

    if ( ! util::endsWith(p, ".hlt") )
        p += ".hlt";

    string full_path = util::findInPaths(p, _libdirs);

    if ( full_path.size() == 0 ) {
        error(util::fmt("cannot find module %s", id->pathAsString()));
        goto error;
    }

    char buf[PATH_MAX];
    if ( ! realpath(full_path.c_str(), buf) ) {
        error(util::fmt("error reading %s: %s", full_path, strerror(errno)));
        goto error;
    }

    if ( debugging("context" ) )
        std::cerr << util::fmt("Found module %s as %s ...", id->pathAsString(), buf) << std::endl;

    return string(buf);

error:
    if ( debugging("context" ) )
        std::cerr << util::fmt("Did not find module %s ...", id->pathAsString()) << std::endl;

    return "";
}

bool CompilerContext::importModule(shared_ptr<ID> id)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Importing module %s ...", id->pathAsString()) << std::endl;

    auto path = searchModule(id);

    if ( ! path.size() )
        return nullptr;

    return (loadModule(path, false, false) != nullptr);
}

shared_ptr<Module> CompilerContext::loadModule(const string& p, bool verify, bool finalize)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Loading module %s ...", p) << std::endl;

    char buf[PATH_MAX];
    if ( ! realpath(p.c_str(), buf) ) {
        error(util::fmt("error reading %s: %s", p, strerror(errno)));
        return nullptr;
    }

    string path(buf);

    auto m = _modules.find(path);

    if ( m != _modules.end() )
        return m->second;

    std::ifstream in(path);

    if ( in.fail() ) {
        error(util::fmt("cannot open input file %s", path.c_str()));
        return nullptr;
    }

    auto module = parse(in, path);

    if ( ! module )
        return nullptr;

    _modules.insert(std::make_pair(path, module));

    if ( finalize ) {
        if ( ! CompilerContext::finalize(verify) )
            return nullptr;
    }

    return module;
}

bool CompilerContext::_finalizeModule(shared_ptr<Module> module, bool verify)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Finalizing module %s ...", module->id()->pathAsString()) << std::endl;

    // Just a double-check ...
    if ( ! InstructionRegistry::globalRegistry()->getAll().size() ) {
        internalError("hilti: no operators defined, did you call hilti_init()?");
    }

    passes::IdResolver          id_resolver;
    passes::InstructionResolver instruction_resolver;
    passes::BlockNormalizer     block_normalizer;
    passes::Validator           validator;

    if ( ! instruction_resolver.run(module) )
        return false;

    if ( ! block_normalizer.run(module) )
        return false;

    // Run these again, we have inserted new instructions.

    if ( ! id_resolver.run(module) )
        return false;

    if ( ! instruction_resolver.run(module) )
        return false;

    if ( verify && ! validator.run(module) )
        return false;

    return true;
}

void CompilerContext::addModule(shared_ptr<Module> module)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Adding module %s ...", module->id()->pathAsString()) << std::endl;

    string path = module->path();

    if ( path == "-" || path == "" )
        path = util::fmt("<no path for %s>", module->id()->pathAsString());

    _modules.insert(std::make_pair(path, module));
}

bool CompilerContext::finalize(bool verify)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Finalizing context...") << std::endl;

    if ( ! _modules.size() )
        return true;

    // This is implictly imported by all modules.
    importModule(std::make_shared<ID>("libhilti"));

    for ( auto m : _modules ) {
        auto module = m.second;

        passes::ScopeBuilder scope_builder(this);
        passes::ScopePrinter scope_printer(std::cerr);

        if ( ! scope_builder.run(module) )
            return false;

        if ( debugging("scopes") )
            scope_printer.run(module);

    }

    for ( auto m : _modules ) {
        auto module = m.second;

        passes::IdResolver id_resolver;
        if ( ! id_resolver.run(module) )
            return false;
    }

    for ( auto m : _modules ) {
        auto module = m.second;

        if ( ! CompilerContext::_finalizeModule(module, verify) )
            goto error;
    }

    _modules.clear();
    return true;

error:
    _modules.clear();
    return false;
}

shared_ptr<Scope> CompilerContext::scopeAlias(shared_ptr<ID> id)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Creating scope alias for module %s ...", id->pathAsString()) << std::endl;

    auto path = searchModule(id);

    if ( ! path.size() )
        return nullptr;

    auto m = _modules.find(path);

    if ( m == _modules.end() )
        internalError(util::fmt("unknown module %s in CompilerContext::scopeAlias", id->pathAsString()));

    auto module = m->second;
    return module->body()->scope()->createAlias();
}

shared_ptr<Module> CompilerContext::parse(std::istream& in, const std::string& sname)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Parsing %s ...", sname) << std::endl;

    hilti_parser::Driver driver;
    driver.forwardLoggingTo(this);
    driver.enableDebug(debugging("scanner"), debugging("parser"));
    return driver.parse(shared_from_this(), in, sname);
}

llvm::Module* CompilerContext::compile(shared_ptr<Module> module, int debug, bool verify, int profile)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Compiling module %s ...", module->id()->pathAsString()) << std::endl;

    codegen::CodeGen cg(module->compilerContext()->libraryPaths());
    return cg.generateLLVM(module, verify, debug, profile);
}

bool CompilerContext::print(shared_ptr<Module> module, std::ostream& out)
{
    passes::Printer printer(out);
    return printer.run(module);
}

bool CompilerContext::printBitcode(llvm::Module* module, std::ostream& out)
{
    hilti::codegen::AssemblyAnnotationWriter annotator;
    llvm::raw_os_ostream llvm_out(out);
    module->print(llvm_out, &annotator);
    return true;
}

void CompilerContext::generatePrototypes(shared_ptr<Module> module, std::ostream& out)
{
    codegen::ProtoGen pg(out);
    return pg.generatePrototypes(module);
}

bool CompilerContext::dump(shared_ptr<Node> ast, std::ostream& out)
{
    ast->dump(out);
    return true;
}

bool CompilerContext::writeBitcode(llvm::Module* module, std::ostream& out)
{
    llvm::raw_os_ostream llvm_out(out);
    llvm::WriteBitcodeToFile(module, llvm_out);
    return true;
}

llvm::Module* CompilerContext::linkModules(string output, std::list<llvm::Module*> modules, path_list paths, std::list<string> libs, path_list bcas, path_list dylds, bool debug, bool verify, bool add_stdlibs, bool add_sharedlibs)
{
    if ( debugging("context" ) ) {
        std::list<string> names;

        for ( auto m : modules )
            names.push_back(m->getModuleIdentifier());

        std::cerr << util::fmt("Linking modules %s ...", util::strjoin(names, ", ")) << std::endl;
    }

    codegen::Linker linker(libs);

    for ( auto l : libs )
        linker.addNativeLibrary(l);

    for ( auto b : bcas )
        linker.addBitcodeArchive(b);

    if ( add_stdlibs ) {

#if 0
        auto type_info = loadModule(configuration().runtime_typeinfo_hlt);

        if ( ! type_info )
            return nullptr;

        auto type_info_llvm = compile(type_info, debug);

        if ( ! type_info_llvm )
            return nullptr;

        modules.push_back(type_info_llvm);
#endif

        if ( debug )
            linker.addBitcodeArchive(configuration().runtime_library_bca_dbg);
        else
            linker.addBitcodeArchive(configuration().runtime_library_bca);

        linker.addNativeLibrary(configuration().runtime_library_a);
    }

    if ( add_sharedlibs ) {
        for ( auto d : configuration().runtime_shared_libraries )
            dylds.push_back(d);
    }

    for ( auto d : dylds ) {
        string errormsg;

        if ( ! util::startsWith(d, configuration().shared_library_prefix) )
            d = configuration().shared_library_prefix + d;

        if ( ! util::endsWith(d, configuration().shared_library_suffix) )
            d = d + configuration().shared_library_suffix;

        if ( ! llvm::sys::DynamicLibrary::getPermanentLibrary(d.c_str(), &errormsg).isValid() ) {
            error(errormsg);
            return nullptr;
        }
    }

    if ( debugging("context" ) ) {
        std::list<string> names;

        for ( auto m : modules )
            names.push_back(m->getModuleIdentifier());

        std::cerr << util::fmt("  Final set modules to link: %s ...", util::strjoin(names, ", ")) << std::endl;
    }

    return linker.link(output, modules, verify, debugging("linker"));
}

#include "lli-mm.cc"

llvm::ExecutionEngine* CompilerContext::jitModule(llvm::Module* module, int optimize)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("JITing module %s ...", module->getModuleIdentifier()) << std::endl;

    auto opt = llvm::CodeGenOpt::Default;

    switch ( optimize ) {
     case 0:
        opt = llvm::CodeGenOpt::None;
        break;

     case 1:
        opt = llvm::CodeGenOpt::Less;
        break;

     case 2:
        opt = llvm::CodeGenOpt::Default;
        break;

     case 3:
        opt = llvm::CodeGenOpt::Aggressive;
        break;

     default:
        error("unsupported optimization level");
        return 0;
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    string errormsg;

    JITMemoryManager *JMM = new LLIMCJITMemoryManager();

    llvm::EngineBuilder builder(module);
    builder.setJITMemoryManager(JMM);
    builder.setEngineKind(llvm::EngineKind::JIT);
    builder.setUseMCJIT(true);
    builder.setOptLevel(opt);
    builder.setErrorStr(&errormsg);

    llvm::TargetOptions Options;
    Options.JITExceptionHandling = false;
    Options.JITEmitDebugInfo = true;
    Options.JITEmitDebugInfoToDisk = true;
    builder.setTargetOptions(Options);

    auto ee = builder.create();
    if ( ! ee ) {
        error(util::fmt("LLVM jit error: %s", errormsg));
        return nullptr;
    }

    ee->DisableLazyCompilation(true);
    ee->runStaticConstructorsDestructors(false);

    return ee;
}

void* CompilerContext::nativeFunction(llvm::ExecutionEngine* ee, llvm::Module* module, const string& function)
{
    if ( debugging("context" ) )
        std::cerr << util::fmt("Getting native function %s ...", function) << std::endl;

    auto func = module->getFunction(function);

    if ( ! func ) {
        error(util::fmt("jit: no function of name %s in module %s", function, module->getModuleIdentifier()));
        return 0;
    }

    auto fp = ee->getPointerToFunction(func);

    if ( ! fp ) {
        error(util::fmt("jit: cannt get pointer to function %s in module %s", function, module->getModuleIdentifier()));
        return 0;
    }

    return fp;
}

const string_list& CompilerContext::libraryPaths() const
{
    return _libdirs;
}

bool CompilerContext::debugging(const string& label)
{
    return _debug_streams.find(label) != _debug_streams.end();
}

bool CompilerContext::enableDebug(const string& label)
{
    if ( ! validDebugStream(label) )
        return false;

    _debug_streams.insert(label);
    return true;
}

bool CompilerContext::enableDebug(std::set<string>& labels)
{
    for ( auto l : labels )
        enableDebug(l);

    return true;
}

std::list<string> CompilerContext::debugStreams()
{
    return { "codegen", "linker", "parser", "scanner", "scopes", "context" };
}

bool CompilerContext::validDebugStream(const string& label)
{
    auto streams = debugStreams();
    return std::find(streams.begin(), streams.end(), label) != streams.end();
}





