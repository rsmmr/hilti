
#ifndef BINPAC_PARSER_DRIVER_H
#define BINPAC_PARSER_DRIVER_H

#include <iostream>
#include <string>

#include "../common.h"
#include "../attribute.h"
#include "../declaration.h"
#include "../ctor.h"
#include "../type.h"
#include "../context.h"
#include "../visitor-interface.h"

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

    binpac::ctor::Map::element map_element;
    binpac::ctor::Map::element_list map_elements;
#if 0
    binpac::ctor::RegExp::pattern re_pattern;
    binpac::ctor::RegExp::pattern_list re_patterns;
#endif

    binpac::Declaration::Linkage linkage;

    shared_ptr<binpac::Attribute> attribute;
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
    shared_ptr<binpac::type::unit::item::Field> unit_field;
    shared_ptr<binpac::type::unit::item::field::switch_::Case> switch_case;
    shared_ptr<binpac::statement::try_::Catch> catch_;
    shared_ptr<binpac::type::integer::Bits> bits_spec;

    binpac::attribute_list attributes;
    binpac::parameter_list params;
    binpac::declaration_list declarations;
    binpac::expression_list expressions;
    binpac::parameter_list parameters;
    binpac::id_list ids;
    binpac::statement_list statements;
    binpac::type_list types;
    binpac::hook_list hooks;
    binpac::unit_item_list unit_items;
    binpac::unit_field_list unit_fields;
    binpac::type::unit::item::field::Switch::case_list switch_cases;
    binpac::type::Integer::bits_list bits;
    binpac::ctor::RegExp::pattern_list re_patterns;
    binpac::ctor::RegExp::pattern re_pattern;
    binpac::type::function::CallingConvention cc;

    std::list<string> strings;
    std::list<std::pair<shared_ptr<binpac::ID>, int>> id_and_ints;

    std::pair<shared_ptr<binpac::Type>, shared_ptr<binpac::Expression>> type_and_expr;
    std::pair<shared_ptr<binpac::ID>, int> id_and_int;
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

class Driver : public ast::Logger {
public:
    shared_ptr<binpac::Module> parse(binpac::CompilerContext* ctx, std::istream& in, const std::string& sname);
    shared_ptr<binpac::Expression> parseExpression(binpac::CompilerContext* ctx, const std::string& expr);

    // Report parsing errors.
    void error(const std::string& m, const binpac_parser::location& l);

    // The Bison parser needs a non-const pointer here. Grmpf.
    std::string* streamName() { return &_sname; }

    Scanner* scanner() const { return _scanner; }
    Parser* parser() const { return _parser; }
    binpac::CompilerContext* context() const { return _context; }

    shared_ptr<binpac::Module> module() const;
    void setModule(shared_ptr<binpac::Module> module);

    shared_ptr<binpac::Expression> expression() const;
    void setExpression(shared_ptr<binpac::Expression> expr);

    void pushScope(shared_ptr<binpac::Scope> scope);
    shared_ptr<binpac::Scope> popScope();
    shared_ptr<binpac::Scope> scope() const;

    void pushBlock(shared_ptr<binpac::statement::Block> block);
    shared_ptr<binpac::statement::Block> popBlock();
    shared_ptr<binpac::statement::Block> block() const;

    void disablePatternMode();
    void enablePatternMode();

    int nextToken();

    /// Enables additional debugging output.
    ///
    /// scanner: True to enable lexer debugging.
    ///
    /// parser: True to enable parser debugging.
    void enableDebug(bool scanner, bool parser);

private:
    std::string _sname;
    binpac::CompilerContext* _context = nullptr;
    shared_ptr<binpac::Module> _module = nullptr;
    shared_ptr<binpac::Expression> _expr = nullptr;

    Scanner* _scanner = 0;
    Parser* _parser = 0;

    bool _dbg_scanner = false;
    bool _dbg_parser = false;

    int _next_token;

    std::list<shared_ptr<binpac::Scope>> _scopes;
    std::list<shared_ptr<binpac::statement::Block>> _blocks;
};

}

#endif
