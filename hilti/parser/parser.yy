%skeleton "lalr1.cc"                          /*  -*- C++ -*- */
%require "2.3"
%defines

%{
namespace hilti_parser { class Parser; }

#include "parser/driver.h"

%}

%locations

%name-prefix="hilti_parser"
%define "parser_class_name" "Parser"

%initial-action
{
    @$.begin.filename = @$.end.filename = driver.streamName();
};

%parse-param { class Driver& driver }
%lex-param   { class Driver& driver }

%error-verbose
%debug
%verbose

%union {}

%{
#include "parser/scanner.h"
#include "builder.h"

#undef yylex
#define yylex driver.scanner()->lex

inline ast::Location loc(const hilti_parser::Parser::location_type& ploc)
{
    if ( ploc.begin.filename == ploc.end.filename )
        return ast::Location(*ploc.begin.filename, ploc.begin.line, ploc.end.line);
    else
        return ast::Location(*ploc.begin.filename, ploc.begin.line);
}

// FIXME: Seems we should be able to use an C++11 initializer_list directly
// instead of calling this function. Doesn't get that to work however,
inline instruction::Operands make_ops(shared_ptr<Expression> op0, shared_ptr<Expression> op1, shared_ptr<Expression> op2, shared_ptr<Expression> op3)
{
    instruction::Operands ops;
    ops.push_back(op0);
    ops.push_back(op1);
    ops.push_back(op2);
    ops.push_back(op3);
    return ops;
}

using namespace hilti;

%}

%token         END     0      "end of file"
%token <sval>  COMMENT        "comment"
%token <sval>  DOTTED_IDENT   "dotted identifier"
%token <sval>  IDENT          "identifier"
%token <sval>  LABEL_IDENT    "block label"
%token <sval>  SCOPED_IDENT   "scoped identifier"
%token         NEWLINE        "newline"

%token <ival>  CINTEGER       "integer"
%token <bval>  CBOOL          "bool"
%token <sval>  CSTRING        "string"
%token <sval>  CBYTES         "bytes"
%token <sval>  CREGEXP        "regular expression"
%token <sval>  CADDRESS       "address"
%token <sval>  CPORT          "port"
%token <dval>  CDOUBLE        "double"

%token <sval>  ATTR_DEFAULT   "'&default'"
%token <sval>  ATTR_GROUP     "'&group'"
%token <sval>  ATTR_LIBHILTI  "'&libhilti'"
%token <sval>  ATTR_NOSUB     "'&nosub'"
%token <sval>  ATTR_PRIORITY  "'&priority'"
%token <sval>  ATTR_SCOPE     "'&scope'"

%token         ADDR           "'addr'"
%token         ANY            "'any'"
%token         BITSET         "'bitset'"
%token         BOOL           "'bool'"
%token         BYTES          "'bytes'"
%token         CADDR          "'caddr'"
%token         CALLABLE       "'callable'"
%token         CATCH          "'catch'"
%token         CHANNEL        "'channel'"
%token         CLASSIFIER     "'classifier'"
%token         CNULL          "'null'"
%token         CONST          "'const'"
%token         DECLARE        "'declare'"
%token         DOUBLE         "'double'"
%token         ENUM           "'enum'"
%token         EXCEPTION      "'exception'"
%token         EXPORT         "'export'"
%token         FILE           "'file'"
%token         GLOBAL         "'global'"
%token         HOOK           "'hook'"
%token         IMPORT         "'import'"
%token         INT            "'int'"
%token         INTERVAL       "'interval'"
%token         IOSRC          "'iosrc'"
%token         ITER           "'iter'"
%token         LIST           "'list'"
%token         LOCAL          "'local'"
%token         MAP            "'map'"
%token         MATCH_TOKEN_STATE "'match_token_state'"
%token         MODULE         "'module'"
%token         NET            "'net'"
%token         OVERLAY        "'overlay'"
%token         PORT           "'port'"
%token         REF            "'ref'"
%token         REGEXP         "'regexp'"
%token         SET            "'set'"
%token         STRING         "'string'"
%token         STRUCT         "'struct'"
%token         TIME           "'time'"
%token         TIMER          "'timer'"
%token         TIMERMGR       "'timer_mgr'"
%token         TRY            "'try'"
%token         TUPLE          "'tuple'"
%token         TYPE           "'type'"
%token         VECTOR         "'vector'"
%token         VOID           "'void'"
%token         WITH           "'with'"
%token         AFTER          "'after'"
%token         AT             "'at'"


%type <bitset_label>     bitset_label
%type <bitset_labels>    bitset_labels
%type <block>            body blocks block_content
%type <bval>             opt_tok_const
%type <catch_>           catch_
%type <catches>          catches
%type <cc>               opt_cc
%type <decl>             global local function hook type_decl
%type <decls>            opt_local_decls
%type <enum_label>       enum_label
%type <enum_labels>      enum_labels
%type <expr>             expr expr_lhs opt_default_expr tuple_elem constant ctor opt_expr
%type <exprs>            opt_tuple_elem_list tuple_elem_list tuple exprs opt_exprs
%type <id>               local_id scoped_id mnemonic import opt_label label
%type <hook_attribute>   hook_attr
%type <hook_attributes>  opt_hook_attrs
%type <map_element>      map_elem
%type <map_elements>     map_elems opt_map_elems
%type <operands>         operands
%type <param>            param result
%type <params>           param_list opt_param_list
%type <re_pattern>       re_pattern 
%type <re_patterns>      ctor_regexp
%type <stmt>             stmt instruction try_catch
%type <stmts>            stmt_list opt_stmt_list first_block more_blocks
%type <strings>          attr_list opt_attr_list
%type <struct_field>     struct_field
%type <struct_fields>    struct_fields opt_struct_fields
%type <overlay_field>    overlay_field
%type <overlay_fields>   overlay_fields opt_overlay_fields
%type <sval>             comment opt_comment opt_exception_libtype attribute re_pattern_constant
%type <type>             base_type type enum_ bitset exception opt_exception_base struct_ overlay
%type <types>            type_list

%%

%start input;

empty_lines   : empty_line empty_lines
              | empty_line
              ;

empty_line    : NEWLINE
              | ';'
              | /* empty */
              ;

eol           : NEWLINE empty_lines
              | ';' empty_lines
              | COMMENT NEWLINE empty_lines
              ;

comment       : COMMENT NEWLINE                  { $$ = $1; }
              | COMMENT NEWLINE comment          { $$ = $1 + $3; }
              ;

opt_comment   : empty_lines comment              { $$ = $2; }
              | empty_lines                      { $$ = ""; }
              ;

opt_nl        : NEWLINE opt_nl
              | /* empty */
              ;

input         : opt_comment module global_decls
              ;

module        : MODULE local_id eol              { auto module = builder::module::create($2, *driver.streamName(), loc(@$));
                                                   driver.context()->module = module;
                                                 }
              ;

local_id      : IDENT                            { $$ = builder::id::create($1, loc(@$)); }

scoped_id     : IDENT                            { $$ = builder::id::create($1, loc(@$)); }
              | SCOPED_IDENT                     { $$ = builder::id::create($1, loc(@$)); }

global_decls  : global_decl global_decls
              | empty_lines
              ;

global_decl   : global                           { driver.context()->module->body()->addDeclaration($1); }
              | stmt                             { driver.context()->module->body()->addStatement($1); }
              | function                         { driver.context()->module->body()->addDeclaration($1); }
              | hook                             { driver.context()->module->body()->addDeclaration($1); }
              | type_decl                        { driver.context()->module->body()->addDeclaration($1); }
              | import                           { driver.context()->module->import($1); }
              | export_                          { }
              | comment                          { }
              ;

opt_local_decls : local opt_local_decls          { $$ = $2; $$.push_front($1); }
              | /* empty */                      { $$ = statement::Block::decl_list(); }
              ;

global        : GLOBAL type local_id eol          { $$ = builder::global::variable($3, $2, 0, loc(@$)); }
              | GLOBAL type local_id '=' expr eol { $$ = builder::global::variable($3, $2, $5, loc(@$)); }
              ;

local         : LOCAL type local_id eol          { $$ = builder::local::variable($3, $2, 0, loc(@$)); }
              | LOCAL type local_id '=' expr eol { $$ = builder::local::variable($3, $2, $5, loc(@$)); }
              ;

type_decl     : TYPE local_id '=' type eol       { $$ = builder::global::type($2, $4, driver.currentScope(), loc(@$)); }

stmt          : instruction eol                  { $$ = $1; }
              ;

stmt_list     : stmt stmt_list                   { $$ = $2; $$.push_front($1); }
              | local stmt_list                  { $$ = $2; driver.currentBlock()->addDeclaration($1); }
              | comment stmt_list                { $$ = $2; }
              | stmt                             { $$ = builder::block::statement_list(); $$.push_back($1); }
              | local                            { $$ = builder::block::statement_list(); driver.currentBlock()->addDeclaration($1); }
              | comment                          { $$ = builder::block::statement_list(); }
              ;

opt_stmt_list : stmt_list                        { $$ = $1; }
              | /* empty */                      { $$ = builder::block::statement_list(); }

instruction   : mnemonic operands                { $$ = builder::instruction::create($1, $2, loc(@$)); }
              | expr_lhs '=' mnemonic operands   { $4[0] = $1; $$ = builder::instruction::create($3, $4, loc(@$)); }
              | expr_lhs '=' expr                { $$ = builder::instruction::create("assign", make_ops($1, $3, nullptr, nullptr), loc(@$)); }
              | try_catch                        { $$ = $1; }

mnemonic      : local_id                         { $$ = $1; }
              | DOTTED_IDENT                     { $$ = builder::id::create($1, loc(@$)); }

try_catch     : TRY body opt_nl catches          { $$ = builder::block::try_($2, $4, loc(@$)); }

catches       : catches catch_ opt_nl            { $$ = $1; $$.push_back($2); }
              | catch_                           { $$ = builder::block::catch_list(); $$.push_back($1); }

catch_        : CATCH '(' type local_id ')' body { $$ = builder::block::catch_($3, $4, $6, loc(@$)); }
              | CATCH body                       { $$ = builder::block::catchAll($2, loc(@$)); }

operands      : /* empty */                      { $$ = make_ops(nullptr, nullptr, nullptr, nullptr); }
              | expr                             { $$ = make_ops(nullptr, $1, nullptr, nullptr); }
              | expr expr                        { $$ = make_ops(nullptr, $1, $2, nullptr); }
              | expr expr expr                   { $$ = make_ops(nullptr, $1, $2, $3); }
              ;

base_type     : ANY                              { $$ = builder::any::type(loc(@$)); }
              | ADDR                             { $$ = builder::address::type(loc(@$)); }
              | BOOL                             { $$ = builder::boolean::type(loc(@$)); }
              | BYTES                            { $$ = builder::bytes::type(loc(@$)); }
              | CADDR                            { $$ = builder::caddr::type(loc(@$)); }
              | DOUBLE                           { $$ = builder::double_::type(loc(@$)); }
              | FILE                             { $$ = builder::file::type(loc(@$)); }
              | INTERVAL                         { $$ = builder::interval::type(loc(@$)); }
              | MATCH_TOKEN_STATE                { $$ = builder::match_token_state::type(loc(@$)); }
              | NET                              { $$ = builder::network::type(loc(@$)); }
              | PORT                             { $$ = builder::port::type(loc(@$)); }
              | STRING                           { $$ = builder::string::type(loc(@$)); }
              | TIME                             { $$ = builder::time::type(loc(@$)); }
              | TIMER                            { $$ = builder::timer::type(loc(@$)); }
              | TIMERMGR                         { $$ = builder::timer_mgr::type(loc(@$)); }
              | VOID                             { $$ = builder::void_::type(loc(@$)); }

              | CHANNEL '<' type '>'             { $$ = builder::channel::type($3, loc(@$)); }
              | INT '<' CINTEGER '>'             { $$ = builder::integer::type($3, loc(@$)); }
              | IOSRC '<' scoped_id '>'          { $$ = builder::iosource::type($3, loc(@$)); }
              | ITER '<' '*' '>'                 { $$ = builder::iterator::typeAny(loc(@$)); }
              | ITER '<' type '>'                { $$ = builder::iterator::type($3, loc(@$)); }
              | LIST '<' type '>'                { $$ = builder::list::type($3, loc(@$)); }
              | LIST '<' '*' '>'                 { $$ = builder::list::typeAny(loc(@$)); }
              | MAP '<' type ',' type '>'        { $$ = builder::map::type($3, $5, loc(@$)); }
              | MAP '<' '*' '>'                  { $$ = builder::map::typeAny(loc(@$)); }
              | REF '<' '*' '>'                  { $$ = builder::reference::typeAny(loc(@$)); }
              | REF '<' type '>'                 { $$ = builder::reference::type($3, loc(@$)); }
              | REGEXP '<' opt_attr_list '>'     { $$ = builder::regexp::type($3, loc(@$)); }
              | REGEXP                           { $$ = builder::regexp::type(std::list<string>(), loc(@$)); }
              | REGEXP '<' '*' '>'               { $$ = builder::regexp::typeAny(loc(@$)); }
              | SET  '<' type '>'                { $$ = builder::set::type($3, loc(@$)); }
              | SET '<' '*' '>'                  { $$ = builder::set::typeAny(loc(@$)); }
              | TUPLE '<' '*' '>'                { $$ = builder::tuple::typeAny(loc(@$)); }
              | TUPLE '<' type_list '>'          { $$ = builder::tuple::type($3, loc(@$)); }
              | VECTOR '<' type '>'              { $$ = builder::vector::type($3, loc(@$)); }
              | VECTOR '<' '*' '>'               { $$ = builder::vector::typeAny(loc(@$)); }
              | CALLABLE '<' type '>'            { $$ = builder::callable::type($3, loc(@$)); }
              | CALLABLE '<' '*' '>'             { $$ = builder::callable::typeAny(loc(@$)); }
              | CLASSIFIER '<' type ',' type '>' { $$ = builder::classifier::type($3, $5, loc(@$)); }
              | CLASSIFIER '<' '*' '>'           { $$ = builder::classifier::typeAny(loc(@$)); }
              | TYPE                             { $$ = builder::type::typeAny(loc(@$)); }

              | bitset                           { $$ = $1; }
              | enum_                            { $$ = $1; }
              | exception                        { $$ = $1; }
              | struct_                          { $$ = $1; }
              | overlay                          { $$ = $1; }
              ;

type          : base_type                        { $$ = $1; }
              | scoped_id                        { $$ = builder::type::byName($1, loc(@$)); }

enum_         : ENUM                             { driver.disableLineMode(); }
                  '{' enum_labels '}'            { driver.enableLineMode(); $$ = builder::enum_::type($4, loc(@$)); }
              ;

enum_labels   : enum_labels ',' enum_label       { $$ = $1; $$.push_back($3); }
              | enum_label                       { $$ = builder::enum_::label_list(); $$.push_back($1); }
              ;

enum_label    : local_id                         { $$ = std::make_pair($1, -1); }
              | local_id '=' CINTEGER            { $$ = std::make_pair($1, $3); }
              ;

bitset        : BITSET                           { driver.disableLineMode(); }
                 '{' bitset_labels '}'           { driver.enableLineMode(); $$ = builder::bitset::type($4, loc(@$)); }
              ;

bitset_labels : bitset_labels ',' bitset_label   { $$ = $1; $$.push_back($3); }
              | bitset_label                     { $$ = builder::bitset::label_list(); $$.push_back($1); }
              ;

bitset_label  : local_id                         { $$ = std::make_pair($1, -1); }
              | local_id '=' CINTEGER            { $$ = std::make_pair($1, $3); }
              ;

exception     : EXCEPTION '<' type '>' opt_exception_base opt_exception_libtype {
                                                 $$ = builder::exception::type($5, $3, loc(@$));
                                                 ast::as<type::Exception>($$)->setLibraryType($6);
                                                 }
              | EXCEPTION opt_exception_base opt_exception_libtype {
                                                 $$ = builder::exception::type($2, nullptr, loc(@$));
                                                 ast::as<type::Exception>($$)->setLibraryType($3);
              }

struct_       : STRUCT                           { driver.disableLineMode(); }
                  '{' opt_struct_fields '}'      { driver.enableLineMode(); $$ = builder::struct_::type($4, loc(@$)); }

opt_struct_fields : struct_fields                { $$ = $1; }
              | /* empty */                      { $$ = builder::struct_::field_list(); }

struct_fields : struct_fields ',' struct_field   { $$ = $1; $$.push_back($3); }
              | struct_field                     { $$ = builder::struct_::field_list(); $$.push_back($1); }

struct_field  : type local_id                    { $$ = builder::struct_::field($2, $1, nullptr, false, loc(@$)); }
              | type local_id ATTR_DEFAULT '=' expr { $$ = builder::struct_::field($2, $1, $5, false, loc(@$)); }

overlay       : OVERLAY                          { driver.disableLineMode(); }
                  '{' opt_overlay_fields '}'     { driver.enableLineMode(); $$ = builder::overlay::type($4, loc(@$)); }

opt_overlay_fields : overlay_fields              { $$ = $1; }
              | /* empty */                      { $$ = builder::overlay::field_list(); }

overlay_fields : overlay_fields ',' overlay_field { $$ = $1; $$.push_back($3); }
              | overlay_field                    { $$ = builder::overlay::field_list(); $$.push_back($1); }

overlay_field : local_id ':' type AT CINTEGER local_id WITH expr opt_expr {
                  if ( $6->name() != "unpack" ) error(@$, "expected 'unpack' in field description");
                  $$ = builder::overlay::field($1, $3, $5, $8, $9, loc(@$));
              }

              | local_id ':' type AFTER local_id local_id WITH expr opt_expr {
                  if ( $6->name() != "unpack" ) error(@$, "expected 'unpack' in field description");
                  $$ = builder::overlay::field($1, $3, $5, $8, $9, loc(@$));
              }


opt_exception_base : ':' type                    { $$ = $2; }
              | /* empty */                      { $$ = nullptr; }

opt_exception_libtype : ATTR_LIBHILTI '=' CSTRING { $$ = $3; }
              | /* empty */                      { $$ = string(); }


opt_attr_list : attr_list                        { $$ = $1; }
              | /* empty */                      { $$ = std::list<string>(); }

attr_list     : attr_list ',' attribute          { $$ = $1; $1.push_back($3); }
              | attribute                        { $$ = std::list<string> {$1}; }

attribute     : ATTR_DEFAULT                     { $$ = $1; }
              | ATTR_GROUP                       { $$ = $1; }
              | ATTR_LIBHILTI                    { $$ = $1; }
              | ATTR_NOSUB                       { $$ = $1; }
              | ATTR_SCOPE                       { $$ = $1; }

type_list     : type ',' type_list               { $$ = $3; $$.push_front($1); }
              | type                             { $$ = builder::type_list(); $$.push_back($1); }
              ;

expr          : constant                         { $$ = $1; }
              | ctor                             { $$ = $1; }
              | scoped_id                        { $$ = builder::id::create($1, loc(@$)); }
              | base_type                        { $$ = builder::type::create($1, loc(@$)); }
              ;

opt_expr      : expr                             { $$ = $1; }
              | /* empty */                      { $$ = nullptr; }

expr_lhs      : scoped_id                        { $$ = builder::id::create($1, loc(@$)); }
              ;

constant      : CINTEGER                         { $$ = builder::integer::create($1, loc(@$)); }
              | CBOOL                            { $$ = builder::boolean::create($1, loc(@$)); }
              | CDOUBLE                          { $$ = builder::double_::create($1, loc(@$)); }
              | CSTRING                          { $$ = builder::string::create($1, loc(@$)); }
              | CNULL                            { $$ = builder::reference::createNull(loc(@$)); }
              | LABEL_IDENT                      { $$ = builder::label::create($1, loc(@$)); }
              | CADDRESS                         { $$ = builder::address::create($1, loc(@$)); }
              | CADDRESS '/' CINTEGER            { $$ = builder::network::create($1, $3, loc(@$)); }
              | CPORT                            { $$ = builder::port::create($1, loc(@$)); }
              | INTERVAL '(' CDOUBLE ')'         { $$ = builder::interval::create($3, loc(@$)); }
              | INTERVAL '(' CINTEGER ')'        { $$ = builder::interval::create($3, loc(@$)); }
              | TIME '(' CDOUBLE ')'             { $$ = builder::time::create($3, loc(@$)); }
              | TIME '(' CINTEGER ')'            { $$ = builder::time::create($3, loc(@$)); }
              | tuple                            { $$ = builder::tuple::create($1, loc(@$));  }
              ;

ctor          : CBYTES                           { $$ = builder::bytes::create($1, loc(@$)); }
              | ctor_regexp                      { $$ = builder::regexp::create($1, loc(@$)); }
              | LIST   '<' type '>' '(' opt_exprs ')' { $$ = builder::list::create($3, $6, loc(@$)); }
              | SET    '<' type '>' '(' opt_exprs ')' { $$ = builder::set::create($3, $6, loc(@$)); }
              | VECTOR '<' type '>' '(' opt_exprs ')' { $$ = builder::vector::create($3, $6, loc(@$)); }
              | MAP    '<' type ',' type '>' '(' opt_map_elems ')' { $$ = builder::map::create($3, $5, $8, loc(@$)); }
              ;

ctor_regexp   : ctor_regexp '|' re_pattern       { $$ = $1; $$.push_back($3); }
              | re_pattern                       { $$ = builder::regexp::re_pattern_list(); $$.push_back($1); }

re_pattern    : re_pattern_constant              { $$ = builder::regexp::pattern($1, ""); }
              | re_pattern_constant IDENT        { $$ = builder::regexp::pattern($1, $2); }

re_pattern_constant : '/' { driver.enablePatternMode(); } CREGEXP { driver.disablePatternMode(); } '/' { $$ = $3; }

opt_map_elems : map_elems                        { $$ = $1; }
              | /* empty */                      { $$ = builder::map::element_list(); }

map_elems     : map_elems ',' map_elem           { $$ = $1; $$.push_back($3); }
              | map_elem                         { $$ = builder::map::element_list(); $$.push_back($1); }

map_elem      : expr ':' expr                    { $$ = builder::map::element($1, $3); }

import        : IMPORT local_id eol              { $$ = $2; }
              ;

export_       : EXPORT local_id eol              { driver.context()->module->exportID($2); }
              | EXPORT type eol                  { driver.context()->module->exportType($2); }
              ;

function      : opt_cc result local_id '(' opt_param_list ')' body opt_nl { $$ = builder::function::create($3, $2, $5, driver.context()->module, $1, $7, loc(@$)); }
              | DECLARE opt_cc result local_id '(' opt_param_list ')' eol { $$ = builder::function::create($4, $3, $6, driver.context()->module, $2, nullptr, loc(@$));
                                                                            driver.context()->module->exportID($4);
                                                                          }
              ;

hook          : HOOK result scoped_id '(' opt_param_list ')' opt_hook_attrs body opt_nl { $$ = builder::hook::create($3, $2, $5, driver.context()->module, $8, $7, loc(@$)); }
              | DECLARE HOOK result local_id '(' opt_param_list ')' eol                 { $$ = builder::hook::create($4, $3, $6, driver.context()->module, nullptr, builder::hook::attributes(), loc(@$));
                                                                                          if ( ! $4->isScoped() )
                                                                                              driver.context()->module->exportID($4, true);
                                                                                        }
              ;

opt_hook_attrs : hook_attr opt_hook_attrs        { $$ = $2; $$.push_back($1); }
               | /* empty */                     { $$ = builder::hook::attributes(); }

hook_attr     : ATTR_PRIORITY '=' CINTEGER       { $$ = builder::hook::attribute($1, $3); }
              | ATTR_GROUP '=' CINTEGER          { $$ = builder::hook::attribute($1, $3); }

opt_cc        : CSTRING                          { if ( $1 == "HILTI" ) $$ = type::function::HILTI;
                                                   else if ( $1 == "C-HILTI" ) $$ = type::function::HILTI_C;
                                                   else if ( $1 == "C" ) $$ = type::function::C;
                                                   else error(@$, "unknown calling convention");
                                                 }
              | /* empty */                      { $$ = type::function::HILTI; }


opt_param_list : param_list                      { $$ = $1; }
               | /* empty */                     { $$ = builder::function::parameter_list(); }

param_list    : param ',' param_list             { $$ = $3; $$.push_front($1); }
              | param                            { $$ = builder::function::parameter_list(); $$.push_back($1); }

param         : opt_tok_const type local_id opt_default_expr { $$ = builder::function::parameter($3, $2, $1, $4, loc(@$)); }

result        : opt_tok_const type               { $$ = builder::function::result($2, $1, loc(@$)); }

opt_tok_const : CONST                            { $$ = true; }
              | /* empty */                      { $$ = false; }

opt_default_expr : '=' expr                      { $$ = $2; }
              | /* empty */                      { $$ = node_ptr<Expression>(); }

body          : opt_nl '{' opt_nl blocks opt_nl '}' { $$ = $4; }

blocks        :                                  { auto b = builder::block::create(driver.context()->module->body()->scope(), loc(@$));
                                                   driver.pushBlock(b);
                                                   driver.pushScope(b->scope());
                                                 }

                first_block                      { $$ = driver.currentBlock();

                                                   for ( auto b : $2 )
                                                       $$->addStatement(b);

                                                   driver.popScope();
                                                   driver.popBlock();
                                                 }

first_block   : opt_label block_content          { $2->setID($1); driver.pushScope($2->scope()); }
                  more_blocks                    { $$ = $4; $$.push_front($2); driver.popScope(); }

              | opt_label block_content          { $2->setID($1); $$ = builder::block::statement_list(); $$.push_front($2); }

more_blocks   : label block_content              { $2->setID($1); driver.pushScope($2->scope()); }
                  more_blocks                    { $$ = $4; $$.push_front($2); driver.popScope(); }

              | label block_content              { $2->setID($1); $$ = builder::block::statement_list(); $$.push_back($2); }
              ;

block_content : opt_local_decls opt_stmt_list    { $$ = builder::block::create($1, $2, driver.currentScope(), loc(@$)); }

opt_label     : label                            { $$ = $1; }
              | /* empty */                      { $$ = nullptr; }

label         : LABEL_IDENT ':' opt_nl           { $$ = builder::id::create($1, loc(@$));; }


tuple         : '(' opt_tuple_elem_list ')'      { $$ = $2; }

opt_tuple_elem_list : tuple_elem_list            { $$ = $1; }
                    | /* empty */                { $$ = builder::tuple::element_list(); }

tuple_elem_list : tuple_elem ',' tuple_elem_list { $$ = $3; $$.push_front($1); }
                | tuple_elem                     { $$ = builder::tuple::element_list(); $$.push_back($1); }

tuple_elem    : expr                             { $$ = $1; }
              | '*'                              { $$ = builder::unset::create(); }

opt_exprs     : exprs                            { $$ = $1; }
              | /* empty */                      { $$ = builder::expression::list(); }

exprs         : exprs ',' expr                   { $$ = $1; $$.push_back($3); }
              | expr                             { $$ = builder::expression::list(); $$.push_back($1); }


%%

void hilti_parser::Parser::error(const Parser::location_type& l, const std::string& m)
{
    driver.error(m, l);
}
