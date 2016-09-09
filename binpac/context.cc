
#include <fstream>
#include <limits.h>

#include <util/util.h>

// LLVM redefines the DEBUG macro. Sigh.
#ifdef DEBUG
#define __SAVE_DEBUG DEBUG
#undef DEBUG
#endif

#include <llvm/IR/Module.h>

#undef DEBUG
#ifdef __SAVE_DEBUG
#define DEBUG __SAVE_DEBUG
#endif

#include "context.h"
#include "expression.h"
#include "operator.h"
#include "options.h"

#include "parser/driver.h"

#include "passes/grammar-builder.h"
#include "passes/id-resolver.h"
#include "passes/normalizer.h"
#include "passes/operator-resolver.h"
#include "passes/overload-resolver.h"
#include "passes/printer.h"
#include "passes/scope-builder.h"
#include "passes/scope-printer.h"
#include "passes/unit-scope-builder.h"
#include "passes/validator.h"

#include "codegen/codegen.h"
#include "codegen/type-builder.h"

#include "hilti/context.h"

using namespace binpac;
using namespace binpac::passes;

binpac::CompilerContext::~CompilerContext()
{
}

const binpac::Options& binpac::CompilerContext::options() const
{
    return *_options;
}

shared_ptr<binpac::Module> binpac::CompilerContext::load(string path, bool finalize)
{
    path = util::strtolower(path);

    if ( ! util::endsWith(path, ".pac2") )
        path += ".pac2";

    string full_path = util::findInPaths(path, options().libdirs_pac2);

    if ( full_path.size() == 0 ) {
        error(util::fmt("cannot find input file %s", path.c_str()));
        return nullptr;
    }

    char buf[PATH_MAX];
    full_path = realpath(full_path.c_str(), buf);

    auto m = _modules.find(full_path);

    if ( m != _modules.end() )
        return m->second;

    std::ifstream in(full_path);

    if ( in.fail() ) {
        error(util::fmt("cannot open input file %s", full_path.c_str()));
        return nullptr;
    }

    auto module = parse(in, full_path);

    if ( ! module )
        return nullptr;

    if ( finalize && ! binpac::CompilerContext::finalize(module) )
        return nullptr;

    _modules.insert(std::make_pair(full_path, module));

    return module;
}


shared_ptr<binpac::Module> binpac::CompilerContext::parse(std::istream& in,
                                                          const std::string& sname)
{
    binpac_parser::Driver driver;
    driver.forwardLoggingTo(this);
    driver.enableDebug(options().cgDebugging("scanner"), options().cgDebugging("parser"));

    _beginPass(sname, "Parser");
    auto module = driver.parse(this, in, sname);
    _endPass();

    return module;
}

shared_ptr<binpac::Expression> binpac::CompilerContext::parseExpression(const std::string& expr)
{
    binpac_parser::Driver driver;
    driver.forwardLoggingTo(this);
    driver.enableDebug(options().cgDebugging("scanner"), options().cgDebugging("parser"));

    auto bexpr = driver.parseExpression(this, expr);

    passes::Normalizer normalizer;

    if ( normalizer.run(bexpr) )
        return bexpr;

    error(util::fmt("error normalizing expression %s", expr));
    return nullptr;
}

static void _debugAST(binpac::CompilerContext* ctx, shared_ptr<binpac::Module> module,
                      const ast::Logger& before)
{
    if ( ctx->options().cgDebugging("dump-ast") ) {
        std::cerr << std::endl
                  << "===" << std::endl
                  << "=== AST for " << module->id()->pathAsString() << " before "
                  << before.loggerName() << std::endl
                  << "===" << std::endl
                  << std::endl;

        ctx->dump(module, std::cerr);
    }

    if ( ctx->options().cgDebugging("print-ast") ) {
        std::cerr << std::endl
                  << "===" << std::endl
                  << "=== AST for " << module->id()->pathAsString() << " before "
                  << before.loggerName() << std::endl
                  << "===" << std::endl
                  << std::endl;

        ctx->print(module, std::cerr);
    }
}

void binpac::CompilerContext::_beginPass(const string& module, const string& name)
{
    hilti::CompilerContext::PassInfo pass;
    pass.module = module;
    pass.time = ::util::currentTime();
    pass.name = name;
    _hilti_context->passes().push_back(pass);
}

void binpac::CompilerContext::_beginPass(shared_ptr<Module> module, const string& name)
{
    _debugAST(this, module, name);
    _beginPass(module->id()->name(), name);
}

void binpac::CompilerContext::_beginPass(shared_ptr<Module> module, const ast::Logger& pass)
{
    _beginPass(module, pass.loggerName());
}

void binpac::CompilerContext::_beginPass(const string& module, const ast::Logger& pass)
{
    _beginPass(module, pass.loggerName());
}

void binpac::CompilerContext::_endPass()
{
    assert(_hilti_context->passes().size());
    auto pass = _hilti_context->passes().back();

    auto cg_time = options().cgDebugging("time");
    auto cgpasses = options().cgDebugging("passes");

    if ( cg_time || cgpasses ) {
        auto delta = util::currentTime() - pass.time;
        auto indent = string(_hilti_context->passes().size(), ' ');

        if ( cgpasses || delta >= 0.1 )
            std::cerr << util::fmt("(%2.2fs) %sbinpac::%s [module \"%s\"]", delta, indent,
                                   pass.name, pass.module)
                      << std::endl;
    }

    _hilti_context->passes().pop_back();
}

bool binpac::CompilerContext::partialFinalize(shared_ptr<Module> module)
{
    return finalize(module, false);
}

bool binpac::CompilerContext::finalize(shared_ptr<Module> module)
{
    // Just a double-check ...
    if ( ! OperatorRegistry::globalRegistry()->byKind(operator_::Plus).size() ) {
        internalError("binpac: no operators defined, did you call binpac::init()?");
    }

    return finalize(module, options().verify);
}

bool binpac::CompilerContext::finalize(shared_ptr<Module> node, bool verify)
{
    passes::GrammarBuilder grammar_builder(std::cerr);
    passes::IDResolver id_resolver;
    passes::OverloadResolver overload_resolver(node);
    passes::Normalizer normalizer;
    passes::OperatorResolver op_resolver(node);
    passes::ScopeBuilder scope_builder(this);
    passes::ScopePrinter scope_printer(std::cerr);
    passes::UnitScopeBuilder unit_scope_builder;
    passes::Validator validator;

    // TODO: This needs a reorg. Currently the op_resolver iterates until it
    // hasn't changed anything anymore but that leaves the other passes out
    // of that loop and we have to run them manually a few times more (and
    // the number of rounds might not be constant actually). We need to pull
    // that up a higher a level and iterate over all the passes until nothing
    // has changed anymore.

    _beginPass(node, scope_builder);

    if ( ! scope_builder.run(node) )
        return false;

    _endPass();

    if ( options().cgDebugging("scopes") )
        scope_printer.run(node);

    _beginPass(node, id_resolver);

    if ( ! id_resolver.run(node, false) )
        return false;

    _endPass();

    _beginPass(node, unit_scope_builder);

    if ( ! unit_scope_builder.run(node) )
        return false;

    _endPass();

    _beginPass(node, id_resolver);

    if ( ! id_resolver.run(node, true) )
        return false;

    _endPass();

    _beginPass(node, overload_resolver);

    if ( ! overload_resolver.run(node) )
        return false;

    _endPass();

    _beginPass(node, op_resolver);

    if ( ! op_resolver.run(node) )
        return false;

    _endPass();

    // The operators may have some unresolved types, too.
    _beginPass(node, id_resolver);
    if ( ! id_resolver.run(node, true) )
        return false;

    _endPass();

    if ( verify ) {
        _beginPass(node, validator);

        if ( ! validator.run(node) )
            return false;

        _endPass();
    }

    _beginPass(node, normalizer);

    if ( ! normalizer.run(node) )
        return false;

    _endPass();

    // Normalizer might have added some new (unresolved) stuff.

    _beginPass(node, id_resolver);
    if ( ! id_resolver.run(node, true) )
        return false;

    _endPass();

    _beginPass(node, overload_resolver);

    if ( ! overload_resolver.run(node) )
        return false;

    _endPass();

    _beginPass(node, op_resolver);

    if ( ! op_resolver.run(node) )
        return false;

    _endPass();

    if ( verify ) {
        _beginPass(node, validator);

        if ( ! validator.run(node) )
            return false;

        _endPass();
    }

    if ( options().cgDebugging("grammars") )
        grammar_builder.enableDebug();

    _beginPass(node, grammar_builder);

    if ( ! grammar_builder.run(node) )
        return false;

    _endPass();

    if ( verify ) {
        _beginPass(node, validator);

        if ( ! validator.run(node) )
            return false;

        _endPass();
    }

    return true;
}

std::unique_ptr<llvm::Module> binpac::CompilerContext::compile(
    shared_ptr<Module> module, shared_ptr<hilti::Module>* hilti_module_out, bool hilti_only)
{
    CodeGen codegen(this);

    _beginPass(module, "CodeGen");

    auto compiled = codegen.compile(module);

    if ( ! compiled )
        return nullptr;

    if ( hilti_module_out )
        *hilti_module_out = compiled;

    _endPass();

    if ( hilti_only )
        return nullptr;

    auto llvm_module = _hilti_context->compile(compiled);

    if ( ! llvm_module )
        return nullptr;

    return llvm_module;
}

std::unique_ptr<llvm::Module> binpac::CompilerContext::compile(
    const string& path, shared_ptr<hilti::Module>* hilti_module_out)
{
    auto module = load(path);

    if ( ! module )
        return nullptr;

    CodeGen codegen(this);

    _beginPass(module, "CodeGen");

    auto compiled = codegen.compile(module);

    if ( hilti_module_out )
        *hilti_module_out = compiled;

    _endPass();

    auto llvm_module = _hilti_context->compile(compiled);

    if ( ! llvm_module )
        return nullptr;

    return llvm_module;
}

bool binpac::CompilerContext::print(shared_ptr<Module> module, std::ostream& out)
{
    passes::Printer printer(out);
    return printer.run(module);
}

bool binpac::CompilerContext::dump(shared_ptr<Node> ast, std::ostream& out)
{
    ast->dump(out);
    return true;
}

shared_ptr<hilti::CompilerContextJIT<binpac::JIT>> binpac::CompilerContext::hiltiContext() const
{
    return _hilti_context;
}

shared_ptr<hilti::Type> binpac::CompilerContext::hiltiType(shared_ptr<binpac::Type> type,
                                                           id_list* deps)
{
    codegen::TypeBuilder tb(nullptr);
    return tb.hiltiType(type, deps);
}

void binpac::CompilerContext::toCacheKey(shared_ptr<Module> module,
                                         ::util::cache::FileCache::Key* key)
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

std::list<string> binpac::CompilerContext::dependencies(shared_ptr<Module> module)
{
    // TODO: Not implementated.
    return std::list<string>();
}
