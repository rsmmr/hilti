
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

struct yystype_hilti {
    bool        bval;
    int64_t     ival;
    double      dval;
    std::string sval;
    hilti::type::function::CallingConvention cc;

    type::Enum::label_list enum_labels;
    type::Enum::Label enum_label;
    type::Bitset::label_list bitset_labels;
    type::Bitset::Label bitset_label;
    type::Scope::field_list scope_fields;
    type::Struct::field_list struct_fields;
    type::Struct::field_list context_fields;
    type::Overlay::field_list overlay_fields;
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
    shared_ptr<hilti::function::Result> result;
    shared_ptr<hilti::function::Parameter> param;
    shared_ptr<hilti::statement::try_::Catch> catch_;
    shared_ptr<hilti::type::struct_::Field> struct_field;
    shared_ptr<hilti::type::struct_::Field> context_field;
    shared_ptr<hilti::type::overlay::Field> overlay_field;

    std::list<shared_ptr<hilti::Type>> types;

    hilti::function::parameter_list params;
    std::list<node_ptr<hilti::Expression>> exprs; // FIXME: This should be replaced with expr2.
    std::list<shared_ptr<hilti::Expression>> exprs2;
    std::list<node_ptr<hilti::statement::try_::Catch>> catches;

    std::list<string> strings;

    hilti::instruction::Operands operands;
};

#define YYSTYPE yystype_hilti

#ifndef __FLEX_LEXER_H
#define yyFlexLexer HiltiFlexLexer
#include <FlexLexer.h>
#undef yyFlexLexer
#endif

namespace hilti {
    class Module;

    namespace builder {
        class ModuleBuilder;
    }
}

namespace hilti_parser {

class Parser;
class Scanner;
class location;

struct ParserContext {
    shared_ptr<hilti::ID> label;
};

class Driver : public ast::Logger {
public:
   shared_ptr<hilti::Module> parse(shared_ptr<CompilerContext> ctx, std::istream& in, const std::string& sname);

   // Report parsing errors.
   void error(const std::string& m, const hilti_parser::location& l);

#if 0
   void checkNotNull(shared_ptr<Node> node, string msg, const hilti_parser::location& l) {
       if ( ! node )
           error(msg, l);
   }
#endif

   // The following methods are used by the parser.

   void begin(shared_ptr<builder::ModuleBuilder> mbuilder) { _mbuilder = mbuilder; }
   void end()                                   {}

   // The Bison parser needs a non-const pointer here. Grmpf.
   std::string* streamName() { return &_sname; }

   // Returns the module builder.
   shared_ptr<builder::ModuleBuilder> moduleBuilder() const { return _mbuilder; }

   Scanner* scanner() const { return _scanner; }
   Parser* parser() const { return _parser; }
   ParserContext* parserContext() const { return _parser_context; }
   shared_ptr<CompilerContext> compilerContext() const { return _compiler_context; }

   void disableLineMode();
   void enableLineMode();
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
   bool _dbg_scanner = false;
   bool _dbg_parser = false;

   ParserContext* _parser_context = 0;
   Scanner* _scanner = 0;
   Parser* _parser = 0;
   shared_ptr<CompilerContext> _compiler_context = 0;
   shared_ptr<builder::ModuleBuilder> _mbuilder = 0;
};

}

#endif
