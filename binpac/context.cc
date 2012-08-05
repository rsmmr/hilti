
#include <fstream>

#include <util/util.h>

#include "context.h"
#include "operator.h"

#include "parser/driver.h"

#include "passes/printer.h"
#include "passes/validator.h"
#include "passes/operator-resolver.h"
#include "passes/id-resolver.h"
#include "passes/scope-builder.h"
#include "passes/scope-printer.h"

using namespace binpac;
using namespace binpac::passes;

CompilerContext::CompilerContext(const string_list& libdirs)
{
    // Always add libbinpac to the path list.
    auto paths = libdirs;
    paths.push_front(".");
    paths.push_front(CONFIG_PATH_LIBBINPAC);

    _libdirs = paths;
}

shared_ptr<Module> CompilerContext::load(string path, bool verify, bool finalize)
{
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
    driver.enableDebug(_dbg_scanner, _dbg_parser);

    return driver.parse(in, sname);
}

bool CompilerContext::finalize(shared_ptr<Module> module, bool verify)
{
    // Just a double-check ...
    if ( ! OperatorRegistry::globalRegistry()->byKind(operator_::Plus).size() ) {
        internalError("binpac: no operators defined, did you call binpac_init()?");
    }

    passes::Validator        validator;
    passes::IDResolver       id_resolver;
    passes::OperatorResolver op_resolver;
    passes::ScopeBuilder     scope_builder(this);
    passes::ScopePrinter     scope_printer(std::cerr);

    if ( ! op_resolver.run(module) )
        return false;

    if ( verify && ! validator.run(module) )
        return false;

    if ( ! scope_builder.run(module) )
        return false;

    if ( _dbg_scopes ) {
        scope_printer.run(module);
    }

    if ( ! id_resolver.run(module) )
        return false;

    if ( verify && ! validator.run(module) )
        return false;

    return true;
}

shared_ptr<hilti::Module> CompilerContext::compile(shared_ptr<Module> module, int debug, bool verify)
{
    std::cerr << "compile() not implemented" << std::endl;
    return nullptr;
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

void CompilerContext::enableDebug(bool scanner, bool parser, bool scopes)
{
    _dbg_scanner = scanner;
    _dbg_parser = parser;
    _dbg_scopes = scopes;
}
