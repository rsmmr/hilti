
#include <fstream>

#include <util/util.h>

#include "context.h"
#include "operator.h"

#include "parser/driver.h"

#include "passes/grammar-builder.h"
#include "passes/id-resolver.h"
#include "passes/overload-resolver.h"
#include "passes/normalizer.h"
#include "passes/operator-resolver.h"
#include "passes/printer.h"
#include "passes/scope-builder.h"
#include "passes/scope-printer.h"
#include "passes/unit-scope-builder.h"
#include "passes/validator.h"

#include "codegen/codegen.h"
#include "codegen/type-builder.h"

using namespace binpac;
using namespace binpac::passes;

shared_ptr<binpac::Module> CompilerContext::load(string path, bool verify, bool finalize)
{
    path = util::strtolower(path);

    if ( ! util::endsWith(path, ".pac2") )
        path += ".pac2";

    string full_path = util::findInPaths(path, _libdirs);

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

    if ( finalize && ! CompilerContext::finalize(module, verify) )
        return nullptr;

    _modules.insert(std::make_pair(full_path, module));

    return module;
}


shared_ptr<Module> CompilerContext::parse(std::istream& in, const std::string& sname)
{
    binpac_parser::Driver driver;
    driver.forwardLoggingTo(this);
    driver.enableDebug(debugging("scanner"), debugging("parser"));

    return driver.parse(this, in, sname);
}

static void _debugAST(CompilerContext* ctx, shared_ptr<Module> module, const ast::Logger& before)
{
    if ( ctx->debugging("dump-ast") ) {
        std::cerr << std::endl
            << "===" << std::endl
            << "=== AST for " << module->id()->pathAsString() << " before " << before.loggerName() << std::endl
            << "===" << std::endl
            << std::endl;

        ctx->dump(module, std::cerr);
    }

    if ( ctx->debugging("print-ast") ) {
        std::cerr << std::endl
            << "===" << std::endl
            << "=== AST for " << module->id()->pathAsString() << " before " << before.loggerName() << std::endl
            << "===" << std::endl
            << std::endl;

        ctx->print(module, std::cerr);
    }
}

bool CompilerContext::finalize(shared_ptr<Module> module, bool verify)
{
    // Just a double-check ...
    if ( ! OperatorRegistry::globalRegistry()->byKind(operator_::Plus).size() ) {
        internalError("binpac: no operators defined, did you call binpac::init()?");
    }

    passes::GrammarBuilder   grammar_builder(std::cerr);
    passes::IDResolver       id_resolver;
    passes::OverloadResolver overload_resolver;
    passes::Normalizer       normalizer;
    passes::OperatorResolver op_resolver;
    passes::ScopeBuilder     scope_builder(this);
    passes::ScopePrinter     scope_printer(std::cerr);
    passes::UnitScopeBuilder unit_scope_builder;
    passes::Validator        validator;

    _debugAST(this, module, scope_builder);

    if ( ! scope_builder.run(module) )
        return false;

    if ( debugging("scopes") )
        scope_printer.run(module);

    _debugAST(this, module, id_resolver);

    if ( ! id_resolver.run(module, false) )
        return false;

    _debugAST(this, module, unit_scope_builder);

    if ( ! unit_scope_builder.run(module) )
        return false;

    _debugAST(this, module, id_resolver);

    if ( ! id_resolver.run(module, true) )
        return false;

    _debugAST(this, module, overload_resolver);

    if ( ! overload_resolver.run(module) )
        return false;

    _debugAST(this, module, op_resolver);

    if ( ! op_resolver.run(module) )
        return false;

    // The operators may have some unresolved types, too.
    _debugAST(this, module, id_resolver);
    if ( ! id_resolver.run(module, true) )
        return false;

    _debugAST(this, module, validator);

    if ( verify ) {
        _debugAST(this, module, validator);

        if ( ! validator.run(module) )
            return false;
    }

    _debugAST(this, module, normalizer);

    if ( ! normalizer.run(module) )
        return false;

    if ( debugging("grammars") )
        grammar_builder.enableDebug();

    _debugAST(this, module, grammar_builder);

    if ( ! grammar_builder.run(module) )
        return false;

    if ( verify ) {
        _debugAST(this, module, validator);

        if ( ! validator.run(module) )
            return false;
    }

    return true;
}

shared_ptr<hilti::Module> CompilerContext::compile(shared_ptr<Module> module, int debug, bool verify)
{
    CodeGen codegen;
    return codegen.compile(module, debug, verify);
}

bool CompilerContext::print(shared_ptr<Module> module, std::ostream& out)
{
    passes::Printer printer(out);
    return printer.run(module);
}

bool CompilerContext::dump(shared_ptr<Node> ast, std::ostream& out)
{
    ast->dump(out);
    return true;
}

const string_list& CompilerContext::libraryPaths() const
{
    return _libdirs;
}

shared_ptr<hilti::CompilerContext> CompilerContext::hiltiContext() const
{
    return _hilti_context;
}

bool CompilerContext::debugging(const string& label)
{
    return _debug_streams.find(label) != _debug_streams.end();
}

bool CompilerContext::enableDebug(std::set<string>& labels)
{
    for ( auto l : labels )
        enableDebug(l);

    return true;
}

std::list<string> CompilerContext::debugStreams()
{
    return { "grammars", "parser", "scanner", "scopes", "dump-ast", "print-ast", "visitors" };
}

bool CompilerContext::validDebugStream(const string& label)
{
    auto streams = debugStreams();
    return std::find(streams.begin(), streams.end(), label) != streams.end();
}

shared_ptr<hilti::Type> binpac::CompilerContext::hiltiType(shared_ptr<binpac::Type> type)
{
    codegen::TypeBuilder tb(nullptr);
    return tb.hiltiType(type);
}
