
#include <fstream>
#include <util/util.h>

// LLVM redefines the DEBUG macro. Sigh.
#ifdef DEBUG
#define __SAVE_DEBUG DEBUG
#undef DEBUG
#endif

#include <llvm/Support/DynamicLibrary.h>

#undef DEBUG
#ifdef __SAVE_DEBUG
#define DEBUG __SAVE_DEBUG
#endif

#include "codegen/asm-annotater.h"
#include "codegen/optimizer.h"
#include "hilti-intern.h"
#include "hilti/autogen/hilti-config.h"
#include "jit.h"
#include "options.h"
#include "parser/driver.h"

using namespace hilti;
using namespace hilti::passes;

CompilerContext::CompilerContext(std::shared_ptr<Options> options)
{
    _llvm_context = new llvm::LLVMContext();
    _llvm_context_owner = true;
    setOptions(options);
}

CompilerContext::~CompilerContext()
{
    if ( _llvm_context_owner )
        delete _llvm_context;
}

llvm::LLVMContext& CompilerContext::llvmContext()
{
    return *_llvm_context;
}

void CompilerContext::setLLVMContext(llvm::LLVMContext* ctx)
{
    if ( _llvm_context_owner )
        delete _llvm_context;

    _llvm_context = ctx;
    _llvm_context_owner = false;
}

const Options& CompilerContext::options() const
{
    return *_options;
}

void CompilerContext::setOptions(std::shared_ptr<Options> options)
{
    _options = options;

    if ( options->module_cache.size() ) {
        if ( options->cgDebugging("cache") )
            std::cerr << "Enabling module cache in " << options->module_cache << std::endl;

        _cache = std::make_shared<::util::cache::FileCache>(options->module_cache);
    }

    else {
        if ( options->cgDebugging("cache") )
            std::cerr << "Disabled module cache" << std::endl;

        _cache = nullptr;
    }

    if ( _options->cgDebugging("visitors") )
        ast::enableDebuggingForAllVisitors(true);
    else
        ast::enableDebuggingForAllVisitors(false);
}

string CompilerContext::searchModule(shared_ptr<ID> id)
{
    auto p = util::strtolower(id->pathAsString());

    if ( ! util::endsWith(p, ".hlt") )
        p += ".hlt";

    string full_path = util::findInPaths(p, options().libdirs_hlt);

    if ( full_path.size() == 0 ) {
        error(util::fmt("cannot find module %s", id->pathAsString()));
        goto error;
    }

    char buf[PATH_MAX];
    if ( ! realpath(full_path.c_str(), buf) ) {
        error(util::fmt("error reading %s: %s", full_path, strerror(errno)));
        goto error;
    }

    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Found module %s as %s ...", id->pathAsString(), buf) << std::endl;

    return string(buf);

error:
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Did not find module %s ...", id->pathAsString()) << std::endl;

    return "";
}

bool CompilerContext::importModule(shared_ptr<ID> id, string* path_out)
{
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Importing module %s ...", id->pathAsString()) << std::endl;

    auto path = searchModule(id);

    if ( ! path.size() )
        return false;

    if ( path_out )
        *path_out = path;

    return (loadModule(path, false) != nullptr);
}

shared_ptr<Module> CompilerContext::loadModule(const string& p, bool finalize)
{
    if ( options().cgDebugging("context") )
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
        if ( ! CompilerContext::finalize(nullptr) )
            return nullptr;
    }

    return module;
}

static void _debugAST(CompilerContext* ctx, shared_ptr<Module> module, const string& before)
{
    if ( ctx->options().cgDebugging("dump-ast") ) {
        std::cerr << std::endl
                  << "===" << std::endl
                  << "=== AST for " << module->id()->pathAsString() << " before hilti::" << before
                  << std::endl
                  << "===" << std::endl
                  << std::endl;

        ctx->dump(module, std::cerr);
    }

    if ( ctx->options().cgDebugging("print-ast") ) {
        std::cerr << std::endl
                  << "===" << std::endl
                  << "=== AST for " << module->id()->pathAsString() << " before hilti::" << before
                  << std::endl
                  << "===" << std::endl
                  << std::endl;

        ctx->print(module, std::cerr);
    }
}

void CompilerContext::_beginPass(const string& module, const string& name)
{
    PassInfo pass;
    pass.module = module;
    pass.time = ::util::currentTime();
    pass.name = name;
    _passes.push_back(pass);
}

void CompilerContext::_beginPass(shared_ptr<Module> module, const string& name)
{
    assert(module);
    _debugAST(this, module, name);
    _beginPass(module->id()->pathAsString(), name);
}

void CompilerContext::_beginPass(shared_ptr<Module> module, const ast::Logger& pass)
{
    _beginPass(module, pass.loggerName());
}

void CompilerContext::_beginPass(const string& module, const ast::Logger& pass)
{
    _beginPass(module, pass.loggerName());
}

void CompilerContext::_endPass()
{
    assert(_passes.size());
    auto pass = _passes.back();

    auto cg_time = options().cgDebugging("time");
    auto cg_passes = options().cgDebugging("passes");

    if ( cg_time || cg_passes ) {
        auto delta = util::currentTime() - pass.time;
        auto indent = string(_passes.size(), ' ');

        if ( cg_passes || delta >= 0.1 )
            std::cerr << util::fmt("(%2.2fs) %shilti::%s [module \"%s\"]", delta, indent, pass.name,
                                   pass.module)
                      << std::endl;
    }

    _passes.pop_back();
}

bool CompilerContext::_finalizeModule(shared_ptr<Module> module, bool verify)
{
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Finalizing module %s ...", module->id()->pathAsString())
                  << std::endl;

    // Just a double-check ...
    if ( ! InstructionRegistry::globalRegistry()->getAll().size() ) {
        internalError("hilti: no operators defined, did you call hilti::init()?");
    }

    passes::IdResolver id_resolver;
    passes::InstructionResolver instruction_resolver;
    passes::InstructionNormalizer instruction_normalizer;
    passes::BlockNormalizer block_normalizer_before_instr(false);
    passes::BlockNormalizer block_normalizer_after_instr(true);
    passes::BlockFlattener block_flattener;
    passes::Validator validator;
    passes::ScopeBuilder scope_builder(this);
    passes::OptimizeCtors optimize_ctors;
    passes::OptimizePeepHole optimize_peephole;

    _beginPass(module, instruction_normalizer);

    if ( ! instruction_normalizer.run(module) )
        return false;

    _endPass();

    _beginPass(module, block_flattener);

    if ( ! block_flattener.run(module) )
        return false;

    _endPass();

    _beginPass(module, block_normalizer_before_instr);

    if ( ! block_normalizer_before_instr.run(module) )
        return false;

    _endPass();

    // Rebuilt scopes.
    _beginPass(module, scope_builder);

    if ( ! scope_builder.run(module) )
        return false;

    _endPass();

    _beginPass(module, id_resolver);

    if ( ! id_resolver.run(module, false) )
        return false;

    _endPass();

    _beginPass(module, instruction_resolver);

    if ( ! instruction_resolver.run(module, false) )
        return false;

    _endPass();

    _beginPass(module, block_normalizer_after_instr);

    if ( ! block_normalizer_after_instr.run(module) )
        return false;

    _endPass();

    // Rebuilt scopes.
    _beginPass(module, scope_builder);

    if ( ! scope_builder.run(module) )
        return false;

    _endPass();

    _beginPass(module, instruction_resolver);

    if ( ! instruction_resolver.run(module, false) )
        return false;

    _endPass();

    // Run these again, we have inserted new instructions.

    _beginPass(module, id_resolver);

    if ( ! id_resolver.run(module, true) )
        return false;

    _endPass();

    _beginPass(module, instruction_resolver);

    if ( ! instruction_resolver.run(module, true) )
        return false;

    _endPass();

    _beginPass(module, validator);

    if ( verify && ! validator.run(module) )
        return false;

    _endPass();

    _beginPass(module, optimize_peephole);

    if ( ! optimize_peephole.run(module) )
        return false;

    _endPass();

    _beginPass(module, optimize_ctors);

    if ( ! optimize_ctors.run(module) )
        return false;

    _endPass();

    _beginPass(module, validator);

    if ( verify && ! validator.run(module) )
        return false;

    _endPass();

    auto cfg = std::make_shared<passes::CFG>(this);
    auto liveness = std::make_shared<passes::Liveness>(this, cfg);

    module->setPasses(cfg, liveness);

    _beginPass(module, *cfg);

    if ( ! cfg->run(module) )
        return false;

    _endPass();

    _beginPass(module, *liveness);

    if ( ! liveness->run(module) )
        return false;

    _endPass();


    return true;
}

void CompilerContext::addModule(shared_ptr<Module> module)
{
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Adding module %s ...", module->id()->pathAsString()) << std::endl;

    string path = module->path();

    if ( path == "-" || path == "" )
        path = util::fmt("<no path for %s>", module->id()->pathAsString());

    for ( auto m : _modules ) {
        if ( m.second == module )
            // Already added.
            return;
    }

    _modules.insert(std::make_pair(path, module));
}

bool CompilerContext::finalize(shared_ptr<Module> module)
{
    return _finalize(module, options().verify);
}

bool CompilerContext::resolveTypes(shared_ptr<Module> module)
{
    passes::GlobalTypeResolver resolver;

    _beginPass(module, resolver);

    if ( ! resolver.run(module) )
        return false;

    _endPass();

    return true;
}

bool CompilerContext::_finalize(shared_ptr<Module> module, bool verify)
{
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Finalizing context...") << std::endl;

    if ( module )
        addModule(module);

    // This is implictly imported by all modules.
    importModule(std::make_shared<ID>("libhilti"));

    for ( auto m : _modules ) {
        auto module = m.second;

        passes::ScopeBuilder scope_builder(this);
        passes::ScopePrinter scope_printer(std::cerr);

        _beginPass(module, scope_builder);

        if ( ! scope_builder.run(module) )
            return false;

        _endPass();

        if ( options().cgDebugging("scopes") ) {
            _beginPass(module, scope_printer);
            scope_printer.run(module);
            _endPass();
        }
    }

    for ( auto m : _modules ) {
        auto module = m.second;

        passes::IdResolver id_resolver;

        _beginPass(module, id_resolver);

        if ( ! id_resolver.run(module, false) )
            return false;

        _endPass();
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
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Creating scope alias for module %s ...", id->pathAsString())
                  << std::endl;

    auto path = searchModule(id);

    if ( ! path.size() )
        return nullptr;

    auto m = _modules.find(path);
    if ( m == _modules.end() )
        internalError(
            util::fmt("cannot find module %s in CompilerContext::scopeAlias", id->pathAsString()));

    auto module = m->second;

    return module->body()->scope()->createAlias();
}

shared_ptr<Module> CompilerContext::parse(std::istream& in, const std::string& sname)
{
    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Parsing %s ...", sname) << std::endl;

    hilti_parser::Driver driver;
    driver.forwardLoggingTo(this);
    driver.enableDebug(options().cgDebugging("scanner"), options().cgDebugging("parser"));

    _beginPass(sname, "Parser");
    auto module = driver.parse(shared_from_this(), in, sname);
    _endPass();

    return module;
}

std::list<std::unique_ptr<llvm::Module>> CompilerContext::checkCache(
    const ::util::cache::FileCache::Key& key)
{
    std::list<std::unique_ptr<llvm::Module>> outputs;
    return outputs;

#if 0
    if ( ! _cache )
        return outputs;

    auto data = _cache->lookup(key);

    int idx = 0;

    for ( auto d : data ) {
        _beginPass(key.name, "LoadFromCache");

        auto mb = llvm::MemoryBuffer::getMemBuffer(d);
        assert(mb);

        string err;
        llvm::LLVMContext llvm_context;

        if ( auto mod = llvm::parseBitcodeFile(mb->getMemBufferRef(), llvm_context) ) {
            if ( options().cgDebugging("cache") )
                std::cerr << util::fmt("Reusing cached module for %s.%s (%d/%d)", key.name,
                                       key.scope, ++idx, data.size())
                          << std::endl;

            _endPass();
            outputs.push_back(mod->get());
        }
        else {
            _endPass();

            if ( options().cgDebugging("cache") )
                std::cerr << util::fmt("Cached module for %s.%s (%d/%d) did not compile", key.name,
                                       key.scope, ++idx, data.size())
                          << std::endl;
        }
    }

    if ( options().cgDebugging("cache") && ! outputs.size() )
        std::cerr << util::fmt("No cached module for %s.%s", key.name, key.scope) << std::endl;

    return outputs;
#endif
}

void CompilerContext::updateCache(const ::util::cache::FileCache::Key& key,
                                  std::unique_ptr<llvm::Module> module)
{
#if 0
    return updateCache(key, {module});
#endif
}

void CompilerContext::updateCache(const ::util::cache::FileCache::Key& key,
                                  std::list<std::unique_ptr<llvm::Module>> modules)
{
    if ( ! _cache )
        return;

    return;

#if 0
    std::list<string> outputs;

    for ( auto m : modules ) {
        if ( options().cgDebugging("cache") )
            std::cerr << "Updating cache for compiled LLVM module " << m->getModuleIdentifier()
                      << std::endl;

        string out;
        llvm::raw_string_ostream llvm_out(out);
        llvm::WriteBitcodeToFile(m, llvm_out);
        outputs.push_back(string(llvm_out.str()));
    }

    _cache->store(key, outputs);
#endif
}

void CompilerContext::toCacheKey(shared_ptr<Module> module, ::util::cache::FileCache::Key* key)
{
    if ( module->path() != "-" )
        key->files.insert(module->path());

    else {
        std::ostringstream s;
        print(module, s);
        auto hash = util::cache::hash(s.str());
        key->hashes.insert(hash);
    }

    for ( auto d : dependencies(module) )
        key->files.insert(d);
}

void CompilerContext::toCacheKey(const llvm::Module* module, ::util::cache::FileCache::Key* key)
{
    string out;
    llvm::raw_string_ostream llvm_out(out);
    llvm::WriteBitcodeToFile(module, llvm_out);

    // We take the size as hash. That's not ideal but (1) the output of
    // neither bitcode nor assemly writer is stable, (2) writing the whole
    // module is also pretty slow.
    auto hash = util::cache::hash(llvm_out.str().size());
    key->hashes.insert(hash);
}

std::list<string> CompilerContext::dependencies(shared_ptr<Module> module)
{
    // TODO: Not implementated.
    return std::list<string>();
}

std::string CompilerContext::llvmGetModuleIdentifier(llvm::Module* module)
{
    return codegen::CodeGen::llvmGetModuleIdentifier(module);
}

std::unique_ptr<llvm::Module> CompilerContext::compile(shared_ptr<Module> module)
{
    // module->dump(std::cerr);

    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("Compiling module %s ...", module->id()->pathAsString())
                  << std::endl;

    _beginPass(module, "CodeGen");

    codegen::CodeGen cg(this, module->compilerContext()->options().libdirs_hlt);
    auto compiled = cg.generateLLVM(module);

    if ( ! compiled )
        return nullptr;

    _endPass();

    // We don't optimize the module here. While that could potentially speed
    // up the final optimization at link time (or not, unknown), it would
    // also change the instructions the linker is looking for to replace.
    // Hence we can optimize only after the linker is done with that.
    //
    // TODO: There might be ways to get the data stable, such as moving them
    // into tiny functions and then having the linker replace the function
    // instead. Not sure if it's worth it, even just doing the LTO passes an
    // link-time (which we want there, not here, obviously), take most of the
    // time.

    return compiled;
}

bool CompilerContext::print(shared_ptr<Module> module, std::ostream& out, bool cfg)
{
    passes::Printer printer(out, false, cfg);
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

std::unique_ptr<llvm::Module> CompilerContext::linkModules(
    string output, std::list<std::unique_ptr<llvm::Module>>& modules, std::list<string> libs,
    path_list bcas, path_list dylds, bool add_stdlibs, bool add_sharedlibs)
{
    if ( options().cgDebugging("context") ) {
        std::list<string> names;

        for ( auto& m : modules )
            names.push_back(llvmGetModuleIdentifier(m.get()));

        std::cerr << util::fmt("Linking modules %s ...", util::strjoin(names, ", ")) << std::endl;
    }

    codegen::Linker linker(this, libs);

    for ( auto l : libs ) {
        if ( options().cgDebugging("context") )
            std::cerr << "Linker: adding native library " << l << std::endl;

        linker.addNativeLibrary(l);
    }

    for ( auto b : bcas ) {
        if ( options().cgDebugging("context") )
            std::cerr << "Linker: adding bitcode library " << b << std::endl;

        linker.addBitcodeFile(b);
    }

    if ( add_stdlibs ) {
#if 0
        auto type_info = loadModule(configuration().runtime_typeinfo_hlt);

        if ( ! type_info )
            return nullptr;

        auto type_info_llvm = compile(type_info);

        if ( ! type_info_llvm )
            return nullptr;

        modules.push_back(type_info_llvm);
#endif

        auto rlbca = options().debug ? configuration().runtime_library_bca_dbg :
                                       configuration().runtime_library_bca;

        if ( options().cgDebugging("context") )
            std::cerr << "Linker: adding bitcode runtime library " << rlbca << std::endl;

        linker.addBitcodeFile(rlbca);

// Linker doesn't support native libraries currently, but it doesn't look like we need it actualy.
#if 0
        if ( options().cgDebugging("context" ) )
            std::cerr << "Linker: adding native runtime library " << configuration().runtime_library_a << std::endl;

        linker.addNativeLibrary(configuration().runtime_library_a);
#endif
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

        if ( options().cgDebugging("context") )
            std::cerr << "Linker: adding dynamic library " << d << std::endl;

        // Sic. This returns true on error ...
        if ( llvm::sys::DynamicLibrary::LoadLibraryPermanently(d.c_str(), &errormsg) ) {
            error(errormsg);
            return nullptr;
        }
    }

    if ( options().cgDebugging("context") ) {
        std::list<string> names;

        for ( auto& m : modules )
            names.push_back(llvmGetModuleIdentifier(m.get()));

        std::cerr << util::fmt("  Final set modules to link: %s ...", util::strjoin(names, ", "))
                  << std::endl;
    }

    _beginPass(output, linker);

    auto linked = linker.link(output, modules);

    if ( ! linked )
        return nullptr;

    _endPass();

    return _optimize(std::move(linked), true);
}

std::unique_ptr<llvm::Module> CompilerContext::_optimize(std::unique_ptr<llvm::Module> module,
                                                         bool is_linked)
{
    if ( ! options().optimize )
        return module;

    if ( options().cgDebugging("context") )
        std::cerr << "Optimizing final linked module ... " << std::endl;

    codegen::Optimizer optimizer(this);

    _beginPass(module->getModuleIdentifier(), optimizer);

    auto nmodule = optimizer.optimize(std::move(module), is_linked);

    _endPass();

    return nmodule;
}

bool CompilerContext::_jit(std::unique_ptr<llvm::Module> module, JIT* jit)
{
    if ( ! options().jit ) {
        error("jitModule() called but options.jit not set\n");
        return false;
    }

    if ( options().cgDebugging("context") )
        std::cerr << util::fmt("JITing module %s ...", module->getModuleIdentifier()) << std::endl;

    _beginPass("<JIT>", "JIT-setup");

    if ( ! jit->jit(std::move(module)) ) {
        error("JITing module failed");
        return false;
    }

    _endPass();

    return true;
}

shared_ptr<util::cache::FileCache> CompilerContext::fileCache() const
{
    return _cache;
}
