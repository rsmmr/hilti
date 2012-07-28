
#include <fstream>

#include <util/util.h>

#include "context.h"

#include "parser/driver.h"

#include "passes/printer.h"
#include "passes/validator.h"

using namespace binpac;
using namespace binpac::passes;

static const string_list _prepareLibDirs(const string_list& libdirs)
{
    // Always add libbinpac to the path list.
    auto paths = libdirs;
    paths.push_front(".");
    paths.push_front(CONFIG_PATH_LIBBINPAC);
    return paths;
}

CompilerContext::CompilerContext()
{
}

shared_ptr<Module> CompilerContext::load(string path, const string_list& libdirs, bool verify, bool finalize)
{
    if ( ! util::endsWith(path, ".pac2") )
        path += ".pac2";

    auto dirs = _prepareLibDirs(libdirs);

    string full_path = util::findInPaths(path, dirs);

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

    if ( finalize && ! CompilerContext::finalize(module, libdirs, verify) )
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

bool CompilerContext::finalize(shared_ptr<Module> module, const string_list& libdirs, bool verify)
{
    passes::Validator        validator;

    if ( verify && ! validator.run(module) )
        return false;

    return true;

#if 0
    passes::ScopeBuilder     scope_builder(_prepareLibDirs(libdirs));
    passes::IdResolver       id_resolver;
    passes::OperatorResolver op_resolver;

    if ( ! scope_builder.run(module) )
        return false;

    if ( ! id_resolver.run(module) )
        return false;

    if ( ! op_resolver.run(module) )
        return false;

    // Run these again, we may have inserted new operators.

    if ( ! id_resolver.run(module) )
        return false;

    if ( ! op_resolver.run(module) )
        return false;

    if ( verify && ! validator.run(module) )
        return false;

    return true;
#endif
}

shared_ptr<hilti::Module> CompilerContext::compile(shared_ptr<Module> module, const string_list& libdirs, int debug, bool verify)
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
