
#include <iostream>

#include "driver.h"
#include "parser.h"
#include "scanner.h"

#include "../builder.h"

using namespace hilti_parser;

shared_ptr<hilti::Module> Driver::parse(std::istream& in, const std::string& sname)
{
    _sname = sname;

    Context context;
    _context = &context;

    Scanner scanner(&in);
    _scanner = &scanner;

    Parser parser(*this);
    _parser = &parser;
    _parser->set_debug_level(0);
    _parser->parse();

    _scanner = 0;
    _parser = 0;

    if ( errors() > 0 )
        return 0;

    assert(context.module);
    return context.module;
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
