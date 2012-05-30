
#ifndef HILTI_PARSER_DRIVER_H
#define HILTI_PARSER_DRIVER_H

#include <iostream>
#include <string>

#include "../common.h"
#include "../function.h"
#include "../variable.h"
#include "../type.h"
#include "../ctor.h"

#undef YY_DECL
#define	YY_DECL						                 \
    hilti_parser::Parser::token_type				 \
    hilti_parser::Scanner::lex(				         \
        hilti_parser::Parser::semantic_type* yylval, \
        hilti_parser::Parser::location_type* yylloc, \
        hilti_parser::Driver& driver                 \
        )

struct yystype {
    bool        bval;
    int64_t     ival;
    double      dval;
    std::string sval;
    hilti::type::function::CallingConvention cc;

    type::Enum::label_list enum_labels;
    type::Enum::Label enum_label;
    type::Bitset::label_list bitset_labels;
    type::Bitset::Label bitset_label;
    type::Struct::field_list struct_fields;
    ctor::Map::element map_element;
    ctor::Map::element_list map_elements;
    ctor::RegExp::pattern re_pattern;
    ctor::RegExp::pattern_list re_patterns;
    Hook::attribute hook_attribute;
    Hook::attribute_list hook_attributes;

    shared_ptr<hilti::Declaration> decl;
    shared_ptr<hilti::Statement> stmt;
    shared_ptr<hilti::statement::Block> block;
    shared_ptr<hilti::Type> type;
    shared_ptr<hilti::ID> id;
    shared_ptr<hilti::Expression> expr;
    shared_ptr<hilti::Constant> constant;
    shared_ptr<hilti::Module> module;
    shared_ptr<hilti::Function> func;
    shared_ptr<hilti::function::Parameter> param;
    shared_ptr<hilti::statement::try_::Catch> catch_;
    shared_ptr<hilti::type::struct_::Field> struct_field;

    std::list<shared_ptr<hilti::Type>> types;

    hilti::function::parameter_list params;
    std::list<node_ptr<hilti::Statement>> stmts;
    std::list<node_ptr<hilti::Declaration>> decls;
    std::list<node_ptr<hilti::Expression>> exprs;
    std::list<node_ptr<hilti::statement::try_::Catch>> catches;

    std::list<string> strings;

    hilti::instruction::Operands operands;
};

#define YYSTYPE yystype

#ifndef __FLEX_LEXER_H
#define yyFlexLexer HiltiFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace hilti {
    class Module;
}

namespace hilti_parser {

class Parser;
class Scanner;
class location;

struct Context {
   node_ptr<hilti::Module> module;
   std::list<shared_ptr<Scope>> scopes;
   std::list<shared_ptr<statement::Block>> blocks;
};

class Driver : public ast::Logger {
public:
   shared_ptr<hilti::Module> parse(std::istream& in, const std::string& sname);

   // Report parsing errors.
   void error(const std::string& m, const hilti_parser::location& l);

   void checkNotNull(shared_ptr<Node> node, string msg, const hilti_parser::location& l) {
       if ( ! node )
           error(msg, l);
   }

   // The following methods are used by the parsing functions.

   // The Bison parser needs a non-const pointer here. Grmpf.
   std::string* streamName() { return &_sname; }

   Scanner* scanner() const { return _scanner; }
   Parser* parser() const { return _parser; }
   Context* context() const { return _context; }

   void pushScope(shared_ptr<Scope> scope) { _context->scopes.push_back(scope); }
   void popScope() { _context->scopes.pop_back(); }

   shared_ptr<Scope> currentScope() const  { return _context->scopes.back(); }

   void pushBlock(shared_ptr<statement::Block> block) { _context->blocks.push_back(block); }
   void popBlock() { _context->blocks.pop_back(); }

   shared_ptr<statement::Block> currentBlock() const  { return _context->blocks.back(); }

   void disableLineMode();
   void enableLineMode();
   void disablePatternMode();
   void enablePatternMode();

private:
   std::string _sname;

   Context* _context = 0;
   Scanner* _scanner = 0;
   Parser* _parser = 0;
};

}

#endif
