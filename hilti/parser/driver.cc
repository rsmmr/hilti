
#include <iostream>

#include "../builder/module.h"
#include "driver.h"
#include "scanner.h"

#include "hilti/autogen/parser.h"

using namespace hilti_parser;

shared_ptr<hilti::Module> Driver::parse(shared_ptr<CompilerContext> ctx, std::istream& in,
                                        const std::string& sname)
{
    _sname = sname;
    _compiler_context = ctx;

    ParserContext context;
    _parser_context = &context;

    Scanner scanner(&in);
    _scanner = &scanner;
    _scanner->set_debug(_dbg_scanner);

    Parser parser(*this);
    _parser = &parser;
    _parser->set_debug_level(_dbg_parser);
    _parser->parse();

    if ( ! _mbuilder )
        return 0;

    auto module = _mbuilder->module();
    auto merrors = _mbuilder->errors();

    _scanner = 0;
    _parser = 0;
    _mbuilder = 0;

    if ( errors() > 0 || merrors > 0 )
        return 0;

    return module;
}

void Driver::error(const std::string& m, const hilti_parser::location& l)
{
    std::ostringstream loc;
    loc << l;
    ast::Logger::error(m, loc.str());
}

void Driver::disableLineMode()
{
    _scanner->disableLineMode();
}

void Driver::enableLineMode()
{
    _scanner->enableLineMode();
}

void Driver::disablePatternMode()
{
    _scanner->disablePatternMode();
}

void Driver::enablePatternMode()
{
    _scanner->enablePatternMode();
}

void Driver::enableDebug(bool scanner, bool parser)
{
    _dbg_scanner = scanner;
    _dbg_parser = parser;
}
