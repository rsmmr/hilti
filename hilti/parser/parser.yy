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
#include "builder/builder.h"

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
%token         AFTER          "'after'"
%token         ANY            "'any'"
%token         AT             "'at'"
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
%token         CONTEXT        "'context'"
%token         DECLARE        "'declare'"
%token         DOUBLE         "'double'"
%token         ENUM           "'enum'"
%token         EXCEPTION      "'exception'"
%token         EXPORT         "'export'"
%token         FILE           "'file'"
%token         FOR            "'for'"
%token         GLOBAL         "'global'"
%token         HOOK           "'hook'"
%token         IMPORT         "'import'"
%token         IN             "'inr'"
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
%token         SCOPE          "'scope'"
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
%token         INIT           "'init'"

%type <bitset_label>     bitset_label
%type <bitset_labels>    bitset_labels
%type <block>            body block_content
%type <bval>             opt_tok_const opt_init
%type <catch_>           catch_
%type <catches>          catches opt_catches
%type <cc>               opt_cc
%type <context_field>    context_field
%type <context_fields>   context_fields opt_context_fields
%type <enum_label>       enum_label
%type <enum_labels>      enum_labels
%type <expr>             expr expr_lhs opt_default_expr tuple_elem constant ctor opt_expr
%type <exprs>            exprs opt_exprs
%type <exprs2>           opt_tuple_elem_list tuple_elem_list tuple
%type <hook_attribute>   hook_attr
%type <hook_attributes>  opt_hook_attrs
%type <id>               local_id scoped_id mnemonic label scope_field
%type <map_element>      map_elem
%type <map_elements>     map_elems opt_map_elems
%type <operands>         operands
%type <overlay_field>    overlay_field
%type <overlay_fields>   overlay_fields opt_overlay_fields
%type <param>            param
%type <result>           result
%type <params>           param_list opt_param_list
%type <re_pattern>       re_pattern
%type <re_patterns>      ctor_regexp
%type <scope_fields>     scope_fields opt_scope_fields
%type <stmt>             stmt instruction try_catch foreach
%type <strings>          attr_list opt_attr_list
%type <struct_field>     struct_field
%type <struct_fields>    struct_fields opt_struct_fields
%type <sval>             opt_exception_libtype attribute re_pattern_constant 
%type <type>             base_type type enum_ bitset exception opt_exception_base struct_ overlay context scope opt_scope_ref
%type <types>            type_list

%%

%start module;

eol           : NEWLINE
              ;

opt_nl        : NEWLINE opt_nl
              | /* empty */
              ;

module        : MODULE local_id eol              { auto moduleBuilder = std::make_shared<builder::ModuleBuilder>(driver.compilerContext(), $2, *driver.streamName(), loc(@$));
                                                   driver.begin(moduleBuilder);
                                                 }

                global_decls                     { driver.end(); }
              ;

local_id      : IDENT                            { $$ = builder::id::node($1, loc(@$)); }

scoped_id     : IDENT                            { $$ = builder::id::node($1, loc(@$)); }
              | SCOPED_IDENT                     { $$ = builder::id::node($1, loc(@$)); }

global_decls  : global_decl global_decls
              | /* empty */
              ;

global_decl   : global
              | const_
              | function
              | hook
              | type_decl
              | context_decl
              | import
              | stmt
              | export_
              ;

global        : GLOBAL type local_id eol          { driver.moduleBuilder()->addGlobal($3, $2, nullptr, false, loc(@$)); }
              | GLOBAL type local_id '=' expr eol { driver.moduleBuilder()->addGlobal($3, $2, $5, false, loc(@$)); }
              ;

const_        : CONST type local_id '=' expr eol  { driver.moduleBuilder()->addConstant($3, $2, $5, false, loc(@$)); }
              ;

local         : LOCAL type local_id eol          { driver.moduleBuilder()->addLocal($3, $2, 0, false, loc(@$)); }
              | LOCAL type local_id '=' expr eol { driver.moduleBuilder()->addLocal($3, $2, $5, false, loc(@$)); }
              ;

type_decl     : TYPE local_id '=' type eol       { driver.moduleBuilder()->addType($2, $4, false, loc(@$)); }

context_decl  : CONTEXT context eol              { driver.moduleBuilder()->addContext($2, loc(@$)); }

stmt          : instruction                      { driver.moduleBuilder()->builder()->addInstruction($1); }
              ;

stmt_list     : stmt opt_stmt_list
              | local opt_stmt_list
              ;

opt_stmt_list : stmt_list
              | /* empty */

instruction   : mnemonic operands eol            { $$ = builder::instruction::create($1, $2, loc(@$)); }
              | expr_lhs '=' mnemonic operands eol { $4[0] = $1; $$ = builder::instruction::create($3, $4, loc(@$)); }
              | expr_lhs '=' expr eol            { $$ = builder::instruction::create("assign", make_ops($1, $3, nullptr, nullptr), loc(@$)); }
              | try_catch                        { $$ = $1; }
              | foreach eol                      { $$ = $1; }

mnemonic      : local_id                         { $$ = $1; }
              | DOTTED_IDENT                     { $$ = builder::id::node($1, loc(@$)); }

try_catch     : TRY body eol catches             { $$ = builder::block::try_($2, $4, loc(@$)); }

catches       : catch_ opt_catches               { $$ = $2; $$.push_front($1); }

opt_catches   : catches                          { $$ = $1; }
              | /* empty */                      { $$ = builder::block::catch_list(); }

catch_        : CATCH '(' type local_id ')' body eol { $$ = builder::block::catch_($3, $4, $6, loc(@$)); }
              | CATCH body                       eol { $$ = builder::block::catchAll($2, loc(@$)); }

foreach       : FOR '(' local_id IN expr ')' body { $$ = builder::loop::foreach($3, $5, $7, loc(@$)); }

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
              | CHANNEL '<' '*' '>'              { $$ = builder::channel::typeAny(loc(@$)); }
              | INT '<' CINTEGER '>'             { $$ = builder::integer::type($3, loc(@$)); }
              | IOSRC '<' scoped_id '>'          { $$ = builder::iosource::type($3, loc(@$)); }
              | IOSRC '<' '*' '>'                { $$ = builder::iosource::typeAny(loc(@$)); }
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
              | scope                            { $$ = $1; }
              | context                          { $$ = $1; }
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

scope         : SCOPE                            { driver.disableLineMode(); }
                 '{' opt_scope_fields '}'        { driver.enableLineMode(); $$ = builder::scope::type($4, loc(@$)); }
              ;

opt_scope_fields : scope_fields                  { $$ = $1; }
              | /* emopty */                     { $$ = builder::scope::field_list(); }

scope_fields  : scope_fields ',' scope_field     { $$ = $1; $$.push_back($3); }
              | scope_field                      { $$ = builder::scope::field_list(); $$.push_back($1); }
              ;

scope_field   : local_id                         { $$ = $1; }

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

context       :                                  { driver.disableLineMode(); }
                  '{' opt_context_fields '}'     { driver.enableLineMode(); $$ = builder::context::type($3, loc(@$)); }

opt_context_fields : context_fields              { $$ = $1; }
                   | /* empty */                 { $$ = builder::context::field_list(); }

context_fields : context_fields ',' context_field { $$ = $1; $$.push_back($3); }
               | context_field                    { $$ = builder::context::field_list(); $$.push_back($1); }

context_field : type local_id                    { $$ = builder::context::field($2, $1, loc(@$)); }

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
              | base_type '(' ')'                { $$ = builder::expression::default_($1, loc(@$)); }
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

import        : IMPORT local_id eol              { driver.moduleBuilder()->importModule($2); }
              ;

export_       : EXPORT local_id eol              { driver.moduleBuilder()->exportID($2); }
              | EXPORT type eol                  { driver.moduleBuilder()->exportType($2); }
              ;

function      : opt_init opt_cc result local_id '(' opt_param_list ')' opt_scope_ref
                                                 {  auto func = driver.moduleBuilder()->pushFunction($4, $3, $6, $2, $8, true, loc(@$));
                                                    if ( $1 ) func->function()->setInitFunction();
                                                 }
                body opt_nl                      {  auto func = driver.moduleBuilder()->popFunction(); }

              | DECLARE opt_cc result local_id '(' opt_param_list ')' eol
                                                 {  driver.moduleBuilder()->declareFunction($4, $3, $6, $2, loc(@$));
                                                    driver.moduleBuilder()->exportID($4);
                                                 }
              ;

opt_init      : INIT                             { $$ = true; }
              | /* empty */                      { $$ = false; }

opt_scope_ref : ATTR_SCOPE '=' local_id          { $$ = builder::type::byName($3, loc(@$)); }
              | /* empty */                      { $$ = nullptr; }

hook          : HOOK result scoped_id '(' opt_param_list ')' opt_scope_ref opt_hook_attrs
                                                 {  driver.moduleBuilder()->pushHook($3, $2, $5, $7, $8, true, loc(@$)); }

                body opt_nl                      {  driver.moduleBuilder()->popHook(); }


              | DECLARE HOOK result local_id '(' opt_param_list ')' eol
                                                 {  driver.moduleBuilder()->declareHook($4, $3, $6, loc(@$)); }
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

body          : opt_nl '{' opt_nl                { driver.moduleBuilder()->pushBody(true, loc(@$)); }
                blocks opt_nl '}'                { auto body = driver.moduleBuilder()->popBody(); $$ = body->block(); }

blocks        :                                  { driver.parserContext()->label = nullptr; }
                block_content more_blocks

              | label                            { driver.parserContext()->label = $1; }
                block_content more_blocks

more_blocks   : label                            { driver.parserContext()->label = $1; }
                block_content more_blocks
              | /* empty */

block_content :                                  { driver.moduleBuilder()->pushBuilder(driver.parserContext()->label, loc(@$)); }
                opt_stmt_list                    { driver.moduleBuilder()->popBuilder(); }

label         : LABEL_IDENT ':' opt_nl           { $$ = builder::id::node($1, loc(@$));; }


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
