
#include <iostream>

#include "../module.h"
#include "../statement.h"
#include "driver.h"
#include "scanner.h"
#include <spicy/autogen/parser.h>

using namespace spicy;
using namespace spicy_parser;

shared_ptr<spicy::Module> Driver::parse(CompilerContext* ctx, std::istream& in,
                                        const std::string& sname)
{
    _sname = sname;
    _context = ctx;
    _next_token = spicy_parser::Parser::token::START_MODULE;

    Scanner scanner(&in);
    _scanner = &scanner;
    _scanner->set_debug(_dbg_scanner);

    Parser parser(*this);
    _parser = &parser;
    _parser->set_debug_level(_dbg_parser);
    _parser->parse();

    _scanner = 0;
    _parser = 0;

    if ( errors() > 0 )
        return 0;

    return module();
}

shared_ptr<spicy::Expression> Driver::parseExpression(spicy::CompilerContext* ctx,
                                                      const std::string& expr)
{
    _sname = util::fmt("<%s>", expr);
    _context = ctx;
    _next_token = spicy_parser::Parser::token::START_EXPR;

    std::stringstream str;
    str << expr;
    Scanner scanner(&str);
    _scanner = &scanner;
    _scanner->set_debug(_dbg_scanner);

    Parser parser(*this);
    _parser = &parser;
    _parser->set_debug_level(_dbg_parser);
    _parser->parse();

    _scanner = 0;
    _parser = 0;

    if ( errors() > 0 )
        return 0;

    return expression();
}

int Driver::nextToken()
{
    int next = _next_token;
    _next_token = 0;
    return next;
}

void Driver::error(const std::string& m, const spicy_parser::location& l)
{
    std::ostringstream loc;
    loc << l;
    ast::Logger::error(m, loc.str());
}

shared_ptr<Module> Driver::module() const
{
    assert(_module);
    return _module;
}

void Driver::setModule(shared_ptr<Module> module)
{
    _module = module;
    pushScope(_module->body()->scope());
}

shared_ptr<Expression> Driver::expression() const
{
    assert(_expr);
    return _expr;
}

void Driver::setExpression(shared_ptr<Expression> expr)
{
    _expr = expr;
}

void Driver::pushScope(shared_ptr<Scope> scope)
{
    _scopes.push_back(scope);
}

shared_ptr<Scope> Driver::popScope()
{
    assert(_scopes.size());
    auto scope = _scopes.back();
    _scopes.pop_back();
    return scope;
}

shared_ptr<Scope> Driver::scope() const
{
    assert(_scopes.size());
    return _scopes.back();
}

void Driver::pushBlock(shared_ptr<statement::Block> block)
{
    _blocks.push_back(block);
    pushScope(block->scope());
}

shared_ptr<statement::Block> Driver::popBlock()
{
    assert(_blocks.size());
    auto b = _blocks.back();
    _blocks.pop_back();
    popScope();
    return b;
}

shared_ptr<statement::Block> Driver::block() const
{
    assert(_blocks.size());
    return _blocks.back();
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
