
#ifndef SPICY_PARSER_DRIVER_H
#define SPICY_PARSER_DRIVER_H

#include <iostream>
#include <string>

#include "../attribute.h"
#include "../common.h"
#include "../context.h"
#include "../ctor.h"
#include "../declaration.h"
#include "../function.h"
#include "../type.h"
#include "../visitor-interface.h"

#undef YY_DECL
#define YY_DECL                                                                                    \
    spicy_parser::Parser::token_type                                                               \
    spicy_parser::Scanner::lex(spicy_parser::Parser::semantic_type* yylval,                        \
                               spicy_parser::Parser::location_type* yylloc,                        \
                               spicy_parser::Driver& driver)

struct yystype_spicy {
    bool bval;
    double dval;
    int64_t ival;
    std::string sval;

    spicy::ctor::Map::element map_element;
    spicy::ctor::Map::element_list map_elements;
    spicy::ctor::Unit::item unit_ctor_item;
    spicy::ctor::Unit::item_list unit_ctor_items;
#if 0
    spicy::ctor::RegExp::pattern re_pattern;
    spicy::ctor::RegExp::pattern_list re_patterns;
#endif

    spicy::Declaration::Linkage linkage;

    shared_ptr<spicy::Attribute> attribute;
    shared_ptr<spicy::Constant> constant;
    shared_ptr<spicy::Ctor> ctor;
    shared_ptr<spicy::Declaration> declaration;
    shared_ptr<spicy::Expression> expression;
    shared_ptr<spicy::Function> function;
    shared_ptr<spicy::Hook> hook;
    shared_ptr<spicy::ID> id;
    shared_ptr<spicy::Module> module;
    shared_ptr<spicy::Statement> statement;
    shared_ptr<spicy::Type> type;
    shared_ptr<spicy::type::function::Parameter> parameter;
    shared_ptr<spicy::type::function::Result> result;
    shared_ptr<spicy::type::unit::Item> unit_item;
    shared_ptr<spicy::type::unit::item::Field> unit_field;
    shared_ptr<spicy::type::unit::item::field::switch_::Case> switch_case;
    shared_ptr<spicy::statement::try_::Catch> catch_;
    shared_ptr<spicy::type::bitfield::Bits> bits_spec;

    spicy::attribute_list attributes;
    spicy::parameter_list params;
    spicy::declaration_list declarations;
    spicy::expression_list expressions;
    spicy::parameter_list parameters;
    spicy::id_list ids;
    spicy::statement_list statements;
    spicy::type_list types;
    spicy::hook_list hooks;
    spicy::unit_item_list unit_items;
    spicy::unit_field_list unit_fields;
    spicy::type::unit::item::field::Switch::case_list switch_cases;
    spicy::type::Bitfield::bits_list bits;
    spicy::ctor::RegExp::pattern_list re_patterns;
    spicy::type::function::CallingConvention cc;
    spicy::Hook::Kind hook_kind;
    spicy::type::unit::item::Field::Kind field_kind;

    std::list<string> strings;
    std::list<std::pair<shared_ptr<spicy::ID>, int>> id_and_ints;

    std::pair<shared_ptr<spicy::Type>, shared_ptr<spicy::Expression>> type_and_expr;
    std::pair<shared_ptr<spicy::ID>, int> id_and_int;
    std::pair<shared_ptr<spicy::ID>, spicy::type::unit::item::Field::Kind> field_name_and_kind;
};

#define YYSTYPE yystype_spicy

#ifndef __FLEX_LEXER_H
#define yyFlexLexer SpicyFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace spicy_parser {

class Parser;
class Scanner;
class location;

class Driver : public ast::Logger {
public:
    shared_ptr<spicy::Module> parse(spicy::CompilerContext* ctx, std::istream& in,
                                    const std::string& sname);
    shared_ptr<spicy::Expression> parseExpression(spicy::CompilerContext* ctx,
                                                  const std::string& expr);

    // Report parsing errors.
    void error(const std::string& m, const spicy_parser::location& l);

    // The Bison parser needs a non-const pointer here. Grmpf.
    std::string* streamName()
    {
        return &_sname;
    }

    Scanner* scanner() const
    {
        return _scanner;
    }
    Parser* parser() const
    {
        return _parser;
    }
    spicy::CompilerContext* context() const
    {
        return _context;
    }

    shared_ptr<spicy::Module> module() const;
    void setModule(shared_ptr<spicy::Module> module);

    shared_ptr<spicy::Expression> expression() const;
    void setExpression(shared_ptr<spicy::Expression> expr);

    void pushScope(shared_ptr<spicy::Scope> scope);
    shared_ptr<spicy::Scope> popScope();
    shared_ptr<spicy::Scope> scope() const;

    void pushBlock(shared_ptr<spicy::statement::Block> block);
    shared_ptr<spicy::statement::Block> popBlock();
    shared_ptr<spicy::statement::Block> block() const;

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
    spicy::CompilerContext* _context = nullptr;
    shared_ptr<spicy::Module> _module = nullptr;
    shared_ptr<spicy::Expression> _expr = nullptr;

    Scanner* _scanner = 0;
    Parser* _parser = 0;

    bool _dbg_scanner = false;
    bool _dbg_parser = false;

    int _next_token;

    std::list<shared_ptr<spicy::Scope>> _scopes;
    std::list<shared_ptr<spicy::statement::Block>> _blocks;
};
}

#endif
