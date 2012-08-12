%skeleton "lalr1.cc"                          /*  -*- C++ -*- */
%require "2.3"
%defines

%{
namespace binpac_parser { class Parser; }

#include "parser/driver.h"

%}

%locations

%name-prefix="binpac_parser"
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

#include "declaration.h"
#include "module.h"
#include "statement.h"
#include "expression.h"
#include "type.h"

#undef yylex
#define yylex driver.scanner()->lex

inline ast::Location loc(const binpac_parser::Parser::location_type& ploc)
{
    if ( ploc.begin.filename == ploc.end.filename )
        return ast::Location(*ploc.begin.filename, ploc.begin.line, ploc.end.line);
    else
        return ast::Location(*ploc.begin.filename, ploc.begin.line);
}

inline shared_ptr<Expression> makeOp(binpac::operator_::Kind kind, const expression_list& ops, const Location& l)
{
    return std::make_shared<expression::UnresolvedOperator>(kind, ops, l);
}

using namespace binpac;

%}

%token <sval>  SCOPED_IDENT   "scoped identifier"
%token <sval>  IDENT          "identifier"
%token <sval>  ATTRIBUTE      "attribute"
%token <sval>  PROPERTY       "property"

%token <ival>  CINTEGER       "integer"
%token <bval>  CBOOL          "bool"
%token <sval>  CSTRING        "string"
%token <sval>  CBYTES         "bytes"
%token <sval>  CREGEXP        "regular expression"
%token <sval>  CADDRESS       "address"
%token <sval>  CPORT          "port"
%token <dval>  CDOUBLE        "double"

%token         ADDR
%token         ANY
%token         ARROW
%token         BITSET
%token         BOOL
%token         BYTES
%token         CATCH
%token         CONST
%token         CONSTANT
%token         DEBUG_
%token         DECLARE
%token         DOUBLE
%token         END            0 "end of file"
%token         ENUM
%token         EXCEPTION
%token         EXPORT
%token         FILE
%token         FOR
%token         FOREACH
%token         GLOBAL
%token         GEQ
%token         IF
%token         IMPORT
%token         IN
%token         INT
%token         INTERVAL
%token         ITER
%token         LIST
%token         LOCAL
%token         MAP
%token         MODULE
%token         NET
%token         ON
%token         PORT
%token         REGEXP
%token         SET
%token         STRING
%token         SWITCH
%token         TIME
%token         TIMER
%token         TRY
%token         TUPLE
%token         TYPE
%token         UNIT
%token         VAR
%token         VECTOR
%token         VOID
%token         SHIFTLEFT
%token         SHIFTRIGHT
%token         PLUSPLUS
%token         MINUSMINUS
%token         MINUSASSIGN
%token         PLUSASSIGN
%token         RETURN
%token         POW
%token         LEQ
%token         NEW
%token         EQ
%token         CADDR
%token         HASATTR
%token         PRINT
%token         AND
%token         CAST
%token         NEQ
%token         MOD
%token         ELSE
%token         OR

// %type <expression>       expr
// %type <expressions>      exprs
//  %type <attributes>       attr_list opt_attr_list
// %type <types>            type_list

%type <constant>         constant
%type <ctor>             ctor
%type <declaration>      global_decl type_decl var_decl const_decl func_decl hook_decl local_decl
%type <declarations>     opt_global_decls opt_local_decls
%type <expression>       expr list_expr opt_list_expr opt_expr opt_unit_field_cond opt_init_expr init_expr id_expr const_expr
%type <expressions>      exprs opt_exprs opt_unit_field_sinks opt_field_args
%type <id>               local_id scoped_id hook_id opt_unit_field_name property_id
%type <statement>        stmt block
%type <statements>       stmts opt_stmts
%type <type>             base_type type enum_ bitset unit unit_field_type
%type <type_and_expr>    type_or_init
%type <id_and_int>       id_with_int
%type <id_and_ints>      id_with_ints
%type <bval>             opt_debug opt_foreach opt_param_const
%type <parameter>        param
%type <result>           rtype
%type <parameters>       params opt_params
%type <hook>             unit_hook
%type <hooks>            opt_unit_hooks
%type <unit_item>        unit_field unit_item unit_prop unit_global_hook unit_var unit_switch
%type <unit_items>       unit_items opt_unit_items
%type <linkage>          opt_linkage
%type <attribute>        type_attr
%type <attributes>       opt_type_attrs
%type <switch_case>      unit_switch_case
%type <switch_cases>     unit_switch_cases
%%

%start module;

module        : MODULE local_id ';'              { auto module = std::make_shared<Module>(driver.context(), $2, *driver.streamName(), loc(@$));
                                                  driver.setModule(module);
                                                 }

                opt_global_decls                 { driver.module()->body()->addDeclarations($5);
                                                   driver.popScope();
                                                 }
              ;

local_id      : IDENT                            { $$ = std::make_shared<ID>($1, loc(@$)); }

scoped_id     : IDENT                            { $$ = std::make_shared<ID>($1, loc(@$)); }
              | SCOPED_IDENT                     { $$ = std::make_shared<ID>($1, loc(@$)); }

hook_id       : local_id                         { $$ = $1; }
              | PROPERTY                         { $$ = std::make_shared<ID>($1, loc(@$)); } /* for %init/%done */

property_id   : PROPERTY                         { $$ = std::make_shared<ID>($1, loc(@$)); }

opt_global_decls
              : global_decl opt_global_decls     { $$ = $2; if ( $1 ) $$.push_front($1); }
              | /* empty */                      { $$ = declaration_list(); }
              ;

global_decl   : type_decl                        { $$ = $1; }
              | var_decl                         { $$ = $1; }
              | const_decl                       { $$ = $1; }
              | func_decl                        { $$ = $1; }
              | hook_decl                        { $$ = $1; }

              | import                           { $$ = nullptr; }
              | stmt                             { driver.module()->body()->addStatement($1); }
              ;

var_decl      : opt_linkage GLOBAL local_id type_or_init ';'
                                                 { auto var = std::make_shared<variable::Global>($3, $4.first, $4.second, loc(@$));
                                                   $$ = std::make_shared<declaration::Variable>($3, $1, var, loc(@$));
                                                 }

const_decl    : opt_linkage CONST  local_id type_or_init ';'
                                                 { auto var = std::make_shared<constant::Expression>($4.first, $4.second, loc(@$));
                                                   $$ = std::make_shared<declaration::Constant>($3, $1, var, loc(@$));
                                                 }

opt_linkage   : EXPORT                           { $$ = Declaration::EXPORTED; }
              | /* empty */                      { $$ = Declaration::PRIVATE; }


type_or_init  : ':' base_type opt_init_expr      { $$ = std::make_tuple($2, $3); }
              | init_expr                        { $$ = std::make_tuple($1->type(), $1); }

opt_init_expr : init_expr                        { $$ = $1; }
              | /* empty */                      { $$ = nullptr; }

init_expr     : '=' expr                         { $$ = $2; }

import        : IMPORT local_id ';'              { driver.module()->import($2); }

type_decl     : opt_linkage TYPE local_id '=' type ';'
                                                 { $$ = std::make_shared<declaration::Type>($3, $1, $5, loc(@$)); }

func_decl     : opt_linkage rtype local_id '(' opt_params ')' block
                                                 { auto ftype = std::make_shared<type::Function>($2, $5, loc(@$));
                                                   auto func  = std::make_shared<Function>($3, ftype, driver.module(), $7, loc(@$));
                                                   $$ = std::make_shared<declaration::Function>(func, $1, loc(@$));
                                                 }

              | DECLARE opt_linkage rtype local_id '(' opt_params ')' ';'
                                                 { auto ftype = std::make_shared<type::Function>($3, $6, loc(@$));
                                                   auto func  = std::make_shared<Function>($4, ftype, driver.module(), nullptr, loc(@$));
                                                   $$ = std::make_shared<declaration::Function>(func, $2, loc(@$));
                                                 }
              ;

hook_decl     : ON opt_linkage rtype local_id '(' opt_params ')' opt_debug opt_foreach block
                                                 { auto ftype = std::make_shared<type::Hook>($3, $6, loc(@$));
                                                   auto func  = std::make_shared<Hook>($4, ftype, driver.module(), $10, 0, $8, $9, loc(@$));
                                                   $$ = std::make_shared<declaration::Hook>(func, $2, loc(@$));
                                                 }

              | DECLARE ON opt_linkage rtype local_id '(' opt_params ')' ';'
                                                 { auto ftype = std::make_shared<type::Hook>($4, $7, loc(@$));
                                                   auto func  = std::make_shared<Hook>($5, ftype, driver.module(), nullptr, 0, false, false, loc(@$));
                                                   $$ = std::make_shared<declaration::Hook>(func, $3, loc(@$));
                                                 }
              ;

params        : param ',' params                 { $$ = $3; $$.push_front($1); }
              | param                            { $$ = parameter_list(); $$.push_back($1); }

opt_params    : params                           { $$ = $1; }
              | /* empty */                      { $$ = parameter_list(); }

param         : opt_param_const local_id ':' type
                                                 { $$ = std::make_shared<type::function::Parameter>($2, $4, $1, nullptr, loc(@$)); }

rtype         : opt_param_const type             { $$ = std::make_shared<type::function::Result>($2, $1, loc(@$)); }

opt_param_const
              : CONST                            { $$ = true; }
              | /* empty */                      { $$ = false; }

stmt          : block                            { $$ = $1; }
              | expr ';'                         { $$ = std::make_shared<statement::Expression>($1, loc(@$)); }
              | PRINT exprs ';'                  { $$ = std::make_shared<statement::Print>($2, loc(@$)); }
              | RETURN opt_expr ';'              { $$ = std::make_shared<statement::Return>($2, loc(@$)); }
              | IF '(' expr ')' stmt             { $$ = std::make_shared<statement::IfElse>($3, $5, nullptr, loc(@$)); }
              | IF '(' expr ')' stmt ELSE stmt   { $$ = std::make_shared<statement::IfElse>($3, $5, $7, loc(@$)); }

stmts         : stmt stmts                       { $$ = $2; $$.push_front($1); }
              | stmt                             { $$ = statement_list(); $$.push_back($1); }

opt_stmts     : stmts                            { $$ = $1; }
              | /* empty */                      { $$ = statement_list(); }

block         : '{'                              { driver.pushBlock(std::make_shared<statement::Block>(driver.scope(), loc(@$))); }
                opt_local_decls                  { driver.block()->addDeclarations($3); }
                opt_stmts '}'                    { driver.popBlock()->addStatements($5); }

opt_local_decls
              : local_decl opt_local_decls       { $$ = $2; $2.push_front($1); }
              | /* empty */                      { $$ = declaration_list(); }

local_decl    : LOCAL local_id ':' type opt_expr { auto v = std::make_shared<variable::Local>($2, $4, $5, loc(@$));
                                                   $$ = std::make_shared<declaration::Variable>($2, Declaration::PRIVATE, v, loc(@$)); }

type          : base_type                        { $$ = $1; }
              | scoped_id                        { $$ = std::make_shared<type::TypeByName>($1, loc(@$)); }

base_type     : ANY                              { $$ = std::make_shared<type::Any>(loc(@$)); }
              | ADDR                             { $$ = std::make_shared<type::Address>(loc(@$)); }
              | BOOL                             { $$ = std::make_shared<type::Bool>(loc(@$)); }
              | BYTES                            { $$ = std::make_shared<type::Bytes>(loc(@$)); }
              | CADDR                            { $$ = std::make_shared<type::CAddr>(loc(@$)); }
              | DOUBLE                           { $$ = std::make_shared<type::Double>(loc(@$)); }
              | FILE                             { $$ = std::make_shared<type::File>(loc(@$)); }
              | INTERVAL                         { $$ = std::make_shared<type::Interval>(loc(@$)); }
              | NET                              { $$ = std::make_shared<type::Network>(loc(@$)); }
              | PORT                             { $$ = std::make_shared<type::Port>(loc(@$)); }
              | STRING                           { $$ = std::make_shared<type::String>(loc(@$)); }
              | TIME                             { $$ = std::make_shared<type::Time>(loc(@$)); }
              | TIMER                            { $$ = std::make_shared<type::Timer>(loc(@$)); }
              | VOID                             { $$ = std::make_shared<type::Void>(loc(@$)); }

              | INT '<' CINTEGER '>'             { $$ = std::make_shared<type::Integer>($3, loc(@$)); }
              | ITER '<' type '>'                { $$ = std::make_shared<type::Iterator>($3, loc(@$)); }
              | LIST '<' type '>'                { $$ = std::make_shared<type::List>($3, loc(@$)); }
              | MAP '<' type ',' type '>'        { $$ = std::make_shared<type::Map>($3, $5, loc(@$)); }
              | REGEXP                           { $$ = std::make_shared<type::RegExp>(attribute_list(), loc(@$)); }
              | SET  '<' type '>'                { $$ = std::make_shared<type::Set>($3, loc(@$)); }
//            | TUPLE '<' type_list '>'          { $$ = std::make_shared<type::Tuple>($3, loc(@$)); }
              | VECTOR '<' type '>'              { $$ = std::make_shared<type::Vector>($3, loc(@$)); }

              | bitset                           { $$ = $1; }
              | enum_                            { $$ = $1; }
              | unit                             { $$ = $1; }
              ;

bitset        : BITSET '{' id_with_ints '}'      { $$ = std::make_shared<type::Bitset>($3, loc(@$)); }

enum_         : ENUM '{' id_with_ints '}'        { $$ = std::make_shared<type::Enum>($3, loc(@$)); }
              ;

id_with_ints  : id_with_ints ',' id_with_int     { $$ = $1; $$.push_back($3); }
              | id_with_int                      { $$ = std::list<std::pair<shared_ptr<ID>, int>>(); $$.push_back($1); }
              ;

id_with_int   : local_id                         { $$ = std::make_pair($1, -1); }
              | local_id '=' CINTEGER            { $$ = std::make_pair($1, $3); }
              ;

unit          : UNIT opt_params '{' opt_unit_items '}'
                                                 { $$ = std::make_shared<type::Unit>($2, $4, loc(@$)); }

unit_items    : unit_item unit_items             { $$ = $2; $$.push_front($1); }
              | unit_item                        { $$ = unit_item_list(); $$.push_back($1); }

opt_unit_items: unit_items                       { $$ = $1;}
              | /* empty */                      { $$ = unit_item_list(); }


unit_item     : unit_var                         { $$ = $1; }
              | unit_field                       { $$ = $1; }
              | unit_switch                      { $$ = $1; }
              | unit_global_hook                 { $$ = $1; }
              | unit_prop                        { $$ = $1; }

unit_var      : VAR local_id ':' base_type opt_unit_hooks ';'
                                                 { $$ = std::make_shared<type::unit::item::Variable>($2, $4, nullptr, $5); }

unit_global_hook : ON hook_id unit_hook          { $$ = std::make_shared<type::unit::item::GlobalHook>($2, $3, loc(@$)); }

unit_prop     : property_id ';'                  { $$ = std::make_shared<type::unit::item::Property>($1, nullptr, loc(@$)); }
              | property_id '=' constant ';'     { $$ = std::make_shared<type::unit::item::Property>($1, $3, loc(@$)); }

unit_field    : opt_unit_field_name unit_field_type opt_field_args opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks ';'
                                                 { $$ = std::make_shared<type::unit::item::field::Type>($1, $2, nullptr, $5, $7, $4, $3, $6, loc(@$)); }

              | const_expr opt_unit_field_cond opt_unit_hooks ';'
                                                 { $$ = std::make_shared<type::unit::item::field::Constant>($1, $2, $3, loc(@$)); }

              | opt_unit_field_name CREGEXP opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks ';'
                                                 { $$ = std::make_shared<type::unit::item::field::RegExp>($2, $1, nullptr, $4, $6, $3, $5, loc(@$)); }

unit_switch   : SWITCH '(' expr ')' '{' unit_switch_cases '}' ';'
                                                 { $$ = std::make_shared<type::unit::item::field::Switch>($3, $6, hook_list(), loc(@$)); }

unit_switch_cases
              : unit_switch_case unit_switch_cases
                                                 { $$ = $2; $$.push_front($1); }
              | unit_switch_case                 { $$ = type::unit::item::field::Switch::case_list(); $$.push_back($1); }

unit_switch_case
              : exprs ARROW unit_field           { $$ = std::make_shared<type::unit::item::field::switch_::Case>($1, $3, loc(@$)); }
              | '*'   ARROW unit_field           { $$ = std::make_shared<type::unit::item::field::switch_::Case>(expression_list(), $3, loc(@$)); }

unit_field_type: type                            { $$ = $1; }

opt_type_attrs: type_attr opt_type_attrs         { $$ = $2; $$.push_front($1); }
              | /* empty */                      { $$ = attribute_list(); }

type_attr     : ATTRIBUTE '(' expr ')'           { $$ = std::make_shared<Attribute>($1, $3, loc(@$)); }

opt_field_args: '(' opt_exprs ')'                { $$ = $2; }

opt_unit_field_name
              : local_id ':'                     { $$ = $1; }
              | ':'                              { $$ = nullptr; }

opt_unit_field_cond
              : IF '(' expr ')'                  { $$ = $3; }
              | /* empty */                      { $$ = nullptr; }

opt_unit_field_sinks
              : ARROW exprs                      { $$ = $2; }
              | /* empty */                      { $$ = expression_list(); }

opt_unit_hooks: unit_hook opt_unit_hooks         { $$ = $2; $2.push_front($1); }
              | /* empty */                      { $$ = hook_list(); }

unit_hook     : opt_debug opt_foreach block      { $$ = std::make_shared<Hook>(nullptr, nullptr, nullptr, $3, 0, $1, $2, loc(@$)); }

opt_debug     : DEBUG_                           { $$ = true; }
              | /* empty */                      { $$ = false; }

opt_foreach   : FOREACH                          { $$ = true; }
              | /* empty */                      { $$ = false; }

constant      : CINTEGER                         { $$ = std::make_shared<constant::Integer>($1, 64, loc(@$)); }
              | CBOOL                            { $$ = std::make_shared<constant::Bool>($1, loc(@$)); }
              | CDOUBLE                          { $$ = std::make_shared<constant::Double>($1, loc(@$)); }
              | CSTRING                          { $$ = std::make_shared<constant::String>($1, loc(@$)); }
              | CADDRESS                         { $$ = std::make_shared<constant::Address>($1, loc(@$)); }
              | CADDRESS '/' CINTEGER            { $$ = std::make_shared<constant::Network>($1, $3, loc(@$)); }
              | CPORT                            { $$ = std::make_shared<constant::Port>($1, loc(@$)); }
              | INTERVAL '(' CDOUBLE ')'         { $$ = std::make_shared<constant::Interval>($3, loc(@$)); }
              | INTERVAL '(' CINTEGER ')'        { $$ = std::make_shared<constant::Interval>((uint64_t)$3, loc(@$)); }
              | TIME '(' CDOUBLE ')'             { $$ = std::make_shared<constant::Time>($3, loc(@$)); }
              | TIME '(' CINTEGER ')'            { $$ = std::make_shared<constant::Time>((uint64_t)$3, loc(@$)); }
//            | tuple                            { $$ = std::make_shared<constant::Tuple>($1, loc(@$));  }
              ;

ctor          : CBYTES                           { $$ = std::make_shared<ctor::Bytes>($1, loc(@$)); }
//            | ctor_regexp                      { $$ = std::make_shared<ctor::Regexp>($1, loc(@$)); }

              | LIST                '(' opt_exprs ')' { $$ = std::make_shared<ctor::List>(nullptr, $3, loc(@$)); }
              | LIST   '<' type '>' '(' opt_exprs ')' { $$ = std::make_shared<ctor::List>($3, $6, loc(@$)); }
              |                     '[' opt_exprs ']' { $$ = std::make_shared<ctor::List>(nullptr, $2, loc(@$)); }

              | SET                 '(' opt_exprs ')' { $$ = std::make_shared<ctor::Set>(nullptr, $3, loc(@$)); }
              | SET    '<' type '>' '(' opt_exprs ')' { $$ = std::make_shared<ctor::Set>($3, $6, loc(@$)); }

              | VECTOR              '(' opt_exprs ')' { $$ = std::make_shared<ctor::Vector>(nullptr, $3, loc(@$)); }
              | VECTOR '<' type '>' '(' opt_exprs ')' { $$ = std::make_shared<ctor::Vector>($3, $6, loc(@$)); }

              ;

expr          : scoped_id                        { $$ = std::make_shared<expression::ID>($1, loc(@$)); }
              | '(' expr ')'                     { $$ = $2; }
              | ctor                             { $$ = std::make_shared<expression::Ctor>($1, loc(@$)); }
              | constant                         { $$ = std::make_shared<expression::Constant>($1, loc(@$)); }
              | CAST '<' type '>' '(' expr ')'   { $$ = std::make_shared<expression::Coerced>($6, $3, loc(@$)); }

              /* Operators */

              | expr '(' opt_list_expr ')'       { $$ = makeOp(operator_::Call, { $1, $3 }, loc(@$)); }
              | expr '[' expr ']'                { $$ = makeOp(operator_::Index, { $1, $3 }, loc(@$)); }
              | expr AND expr                    { $$ = makeOp(operator_::LogicalAnd, { $1, $3 }, loc(@$)); }
              | expr OR expr                     { $$ = makeOp(operator_::LogicalOr, { $1, $3 }, loc(@$)); }
              | expr '&' expr                    { $$ = makeOp(operator_::BitAnd, { $1, $3 }, loc(@$)); }
              | expr '|' expr                    { $$ = makeOp(operator_::BitOr, { $1, $3 }, loc(@$)); }
              | expr '^' expr                    { $$ = makeOp(operator_::BitXor, { $1, $3 }, loc(@$)); }
              | expr SHIFTLEFT expr              { $$ = makeOp(operator_::ShiftLeft, { $1, $3 }, loc(@$)); }
              | expr SHIFTRIGHT expr             { $$ = makeOp(operator_::ShiftRight, { $1, $3 }, loc(@$)); }
              | expr POW expr                    { $$ = makeOp(operator_::Power, { $1, $3 }, loc(@$)); }
              | expr '.' id_expr                 { $$ = makeOp(operator_::Attribute, { $1, $3 }, loc(@$)); }
              | expr HASATTR id_expr             { $$ = makeOp(operator_::HasAttribute, { $1, $3 }, loc(@$)); }
              | expr PLUSPLUS                    { $$ = makeOp(operator_::IncrPostfix, { $1 }, loc(@$)); }
              | PLUSPLUS expr                    { $$ = makeOp(operator_::IncrPrefix, { $2 }, loc(@$)); }
              | expr MINUSMINUS                  { $$ = makeOp(operator_::DecrPostfix, { $1 }, loc(@$)); }
              | MINUSMINUS expr                  { $$ = makeOp(operator_::DecrPrefix, { $2 }, loc(@$)); }
              | '*' expr                         { $$ = makeOp(operator_::Deref, { $2 }, loc(@$)); }
              | '!' expr                         { $$ = makeOp(operator_::Not, { $2 }, loc(@$)); }
              | '|' expr '|'                     { $$ = makeOp(operator_::Size, { $2 }, loc(@$)); }
              | expr '=' expr                    { $$ = makeOp(operator_::Assign, { $1, $3 }, loc(@$)); }
              | expr '+' expr                    { $$ = makeOp(operator_::Plus, { $1, $3 }, loc(@$)); }
              | expr '-' expr                    { $$ = makeOp(operator_::Minus, { $1, $3 }, loc(@$)); }
              | expr '*' expr                    { $$ = makeOp(operator_::Mult, { $1, $3 }, loc(@$)); }
              | expr '/' expr                    { $$ = makeOp(operator_::Div, { $1, $3 }, loc(@$)); }
              | expr MOD expr                    { $$ = makeOp(operator_::Mod, { $1, $3 }, loc(@$)); }
              | expr EQ expr                     { $$ = makeOp(operator_::Equal, { $1, $3 }, loc(@$)); }
              | expr '<' expr                    { $$ = makeOp(operator_::Lower, { $1, $3 }, loc(@$)); }
              | expr PLUSASSIGN expr             { $$ = makeOp(operator_::PlusAssign, { $1, $3 }, loc(@$)); }
              | expr MINUSASSIGN expr            { $$ = makeOp(operator_::PlusAssign, { $1, $3 }, loc(@$)); }
              | expr '[' expr ']' '=' expr       { $$ = makeOp(operator_::IndexAssign, { $1, $3, $6 }, loc(@$)); }
              | expr '.' id_expr '(' opt_list_expr ')' { $$ = makeOp(operator_::MethodCall, { $1, $3, $5 }, loc(@$)); }
              | NEW id_expr                      { $$ = makeOp(operator_::New, {}, loc(@$)); }
              | NEW id_expr '(' list_expr ')'    { $$ = makeOp(operator_::New, {$2, $4}, loc(@$)); }

              /* Operators derived from other operators. */

              | expr NEQ expr                    { auto e = makeOp(operator_::Equal, { $1, $3 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e }, loc(@$));
                                                 }

              | expr LEQ expr                    { auto e = makeOp(operator_::Greater, { $1, $3 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e }, loc(@$));
                                                 }

              | expr '>' expr                    { auto e1 = makeOp(operator_::Lower, { $1, $3 }, loc(@$));
                                                   auto e2 = makeOp(operator_::Equal, { $1, $3 }, loc(@$));
                                                   auto e3 = makeOp(operator_::LogicalAnd, { e1, e2 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e3 }, loc(@$));
                                                 }

opt_expr      : expr                             { $$ = $1; }
              | /* empty */                      { $$ = nullptr; }

id_expr       : local_id                         { $$ = std::make_shared<expression::ID>($1, loc(@$)); }

const_expr    : constant                         { $$ = std::make_shared<expression::Constant>($1, loc(@$)); }

exprs         : expr ',' exprs                   { $$ = $3; $$.push_front($1); }
              | expr                             { $$ = expression_list(); $$.push_back($1); }

opt_exprs     : exprs                            { $$ = $1; }
              | /* empty */                      { $$ = expression_list(); }

list_expr     : exprs                            { $$ = std::make_shared<expression::List>($1, loc(@$)); ; }

opt_list_expr : list_expr                        { $$ = $1; }
              | /* empty */                      { $$ = std::make_shared<expression::List>(expression_list(), loc(@$)); }

%%

void binpac_parser::Parser::error(const Parser::location_type& l, const std::string& m)
{
    driver.error(m, l);
}
