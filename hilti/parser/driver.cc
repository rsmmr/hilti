
#include <iostream>

#include "driver.h"
#include "scanner.h"
#include "autogen/parser.h"

using namespace hilti_parser;

shared_ptr<hilti::Module> Driver::parse(std::istream& in, const std::string& sname)
{
    _sname = sname;

    Context context;
    _context = &context;

    Scanner scanner(&in);
    _scanner = &scanner;
    _scanner->set_debug(0);

    Parser parser(*this);
    _parser = &parser;
    _parser->set_debug_level(0);
    _parser->parse();

    auto module = _mbuilder->module();

    _scanner = 0;
    _parser = 0;
    _mbuilder = 0;

    if ( errors() > 0 )
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
