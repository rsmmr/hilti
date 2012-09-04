
#ifndef BINPAC_PARSER_DRIVER_H
#define BINPAC_PARSER_DRIVER_H

#include <iostream>
#include <string>

#include "common.h"
#include "attribute.h"
#include "declaration.h"
#include "ctor.h"
#include "type.h"
#include "context.h"
#include "visitor-interface.h"

#undef YY_DECL
#define	YY_DECL						                 \
    binpac_parser::Parser::token_type				 \
    binpac_parser::Scanner::lex(				         \
        binpac_parser::Parser::semantic_type* yylval, \
        binpac_parser::Parser::location_type* yylloc, \
        binpac_parser::Driver& driver                 \
        )

struct yystype_binpac {
    bool        bval;
    double      dval;
    int64_t     ival;
    std::string sval;

#if 0
    ctor::Map::element map_element;
    ctor::Map::element_list map_elements;
    ctor::RegExp::pattern re_pattern;
    ctor::RegExp::pattern_list re_patterns;
#endif

    Declaration::Linkage linkage;

    shared_ptr<Attribute> attribute;
    shared_ptr<binpac::Constant> constant;
    shared_ptr<binpac::Ctor> ctor;
    shared_ptr<binpac::Declaration> declaration;
    shared_ptr<binpac::Expression> expression;
    shared_ptr<binpac::Function> function;
    shared_ptr<binpac::Hook> hook;
    shared_ptr<binpac::ID> id;
    shared_ptr<binpac::Module> module;
    shared_ptr<binpac::Statement> statement;
    shared_ptr<binpac::Type> type;
    shared_ptr<binpac::type::function::Parameter> parameter;
    shared_ptr<binpac::type::function::Result> result;
    shared_ptr<binpac::type::unit::Item> unit_item;
    shared_ptr<binpac::type::unit::item::field::switch_::Case> switch_case;
    shared_ptr<binpac::statement::try_::Catch> catch_;

    attribute_list attributes;
    parameter_list params;
    declaration_list declarations;
    expression_list expressions;
    parameter_list parameters;
    id_list ids;
    statement_list statements;
    type_list types;
    hook_list hooks;
    unit_item_list unit_items;
    binpac::type::unit::item::field::Switch::case_list switch_cases;
    ctor::RegExp::pattern_list re_patterns;
    ctor::RegExp::pattern re_pattern;

    std::list<string> strings;
    std::list<std::pair<shared_ptr<ID>, int>> id_and_ints;

    std::pair<shared_ptr<Type>, shared_ptr<Expression>> type_and_expr;
    std::pair<shared_ptr<ID>, int> id_and_int;
};

#define YYSTYPE yystype_binpac

#ifndef __FLEX_LEXER_H
#define yyFlexLexer BinPACFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace binpac_parser {

class Parser;
class Scanner;
class location;

class Driver : public Logger {
public:
    shared_ptr<binpac::Module> parse(shared_ptr<CompilerContext> ctx, std::istream& in, const std::string& sname);

    // Report parsing errors.
    void error(const std::string& m, const binpac_parser::location& l);

    // The Bison parser needs a non-const pointer here. Grmpf.
    std::string* streamName() { return &_sname; }

    Scanner* scanner() const { return _scanner; }
    Parser* parser() const { return _parser; }
    shared_ptr<CompilerContext> context() const { return _context; }

    shared_ptr<Module> module() const;
    void setModule(shared_ptr<Module> module);

    void pushScope(shared_ptr<Scope> scope);
    shared_ptr<Scope> popScope();
    shared_ptr<Scope> scope() const;

    void pushBlock(shared_ptr<statement::Block> block);
    shared_ptr<statement::Block> popBlock();
    shared_ptr<statement::Block> block() const;

    void disablePatternMode();
    void enablePatternMode();

    /// Enables additional debugging output.
    ///
    /// scanner: True to enable lexer debugging.
    ///
    /// parser: True to enable parser debugging.
    void enableDebug(bool scanner, bool parser);

private:
    std::string _sname;
    shared_ptr<CompilerContext> _context = nullptr;
    shared_ptr<Module> _module = nullptr;

    Scanner* _scanner = 0;
    Parser* _parser = 0;

    bool _dbg_scanner = false;
    bool _dbg_parser = false;

    std::list<shared_ptr<Scope>> _scopes;
    std::list<shared_ptr<statement::Block>> _blocks;
};

}

#endif
