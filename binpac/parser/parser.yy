%skeleton "lalr1.cc"                          /*  -*- C++ -*- */
%require "2.3"
%defines

%{
namespace binpac_parser { class Parser; }

#include <binpac/parser/driver.h>

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
#include <binpac/parser/scanner.h>

#include <binpac/declaration.h>
#include <binpac/module.h>
#include <binpac/statement.h>
#include <binpac/expression.h>
#include <binpac/type.h>
#include <binpac/scope.h>

#undef yylex
#define yylex driver.scanner()->lex

using namespace binpac;

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

inline shared_ptr<type::unit::item::Field> makeVectorField(shared_ptr<type::unit::item::Field> elem,
                                                           shared_ptr<ID> name,
                                                           shared_ptr<Expression> length,
                                                           const hook_list& hooks,
                                                           const Location& l
                                                           )
{
    return std::make_shared<type::unit::item::field::container::Vector>(name, elem, length, nullptr, hooks, attribute_list(), expression_list(), l);
}

%}

%token <sval>  SCOPED_IDENT   "scoped identifier"
%token <sval>  DOLLAR_IDENT   "$-identifier"
%token <sval>  PATH_IDENT     "path"
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

%token         ADD
%token         ADDR
%token         ANY
%token         ARROW
%token         BITFIELD
%token         BITSET
%token         BOOL
%token         BYTES
%token         CATCH
%token         CLEAR
%token         CONST
%token         CONSTANT
%token         DEBUG_
%token         DELETE
%token         DOTDOT
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
%token         INT8
%token         INT16
%token         INT32
%token         INT64
%token         INTERVAL
%token         ITER
%token         LIST
%token         LOCAL
%token         MAP
%token         MARK
%token         MODULE
%token         NET
%token         ON
%token         PORT
%token         PRIORITY
%token         REGEXP
%token         SET
%token         SINK
%token         STRING
%token         SWITCH
%token         TIME
%token         TIMER
%token         TRY
%token         TUPLE
%token         TYPE
%token         UINT
%token         UINT8
%token         UINT16
%token         UINT32
%token         UINT64
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
%token         TRYATTR
%token         PRINT
%token         AND
%token         CAST
%token         NEQ
%token         MOD
%token         ELSE
%token         OR
%token         STOP
%token         OBJECT
%token         OPTIONAL
%token         NONE

%token         ATTR_HILTI_ID

// %type <expression>       expr
// %type <expressions>      exprs
//  %type <attributes>       attr_list opt_attr_list

%type <bits>             bitfield_bits
%type <bits_spec>        bitfield_bits_spec
%type <cc>               opt_cc
%type <constant>         constant tuple optional
%type <ctor>             ctor
%type <declaration>      global_decl type_decl var_decl const_decl func_decl hook_decl local_decl
%type <declarations>     opt_global_decls opt_local_decls
%type <expression>       expr expr2 opt_expr opt_unit_field_cond opt_init_expr init_expr id_expr member_expr tuple_expr opt_unit_vector_len opt_unit_switch_expr
%type <expressions>      exprs opt_exprs opt_unit_field_sinks opt_field_args
%type <id>               local_id scoped_id dollar_id path_id hook_id opt_unit_field_name opt_hilti_id
%type <statement>        stmt block
%type <statements>       stmts opt_stmts
%type <type>             base_type type enum_ bitset unit atomic_type container_type bitfield
%type <type_and_expr>    type_or_init type_or_opt_init
%type <id_and_int>       id_with_int
%type <id_and_ints>      id_with_ints
%type <bval>             opt_debug opt_foreach opt_param_const opt_param_clear
%type <parameter>        param
%type <result>           rtype
%type <parameters>       params opt_params opt_unit_params
%type <hook>             unit_hook
%type <hooks>            unit_hooks opt_unit_hooks
%type <unit_item>        unit_item unit_prop unit_global_hook unit_var unit_switch
%type <unit_items>       unit_items opt_unit_items
%type <unit_field>       unit_field unit_field_in_container
%type <unit_fields>      unit_fields
%type <linkage>          opt_linkage
%type <attribute>        type_attr property
%type <attributes>       opt_type_attrs
%type <switch_case>      unit_switch_case
%type <switch_cases>     unit_switch_cases
%type <re_patterns>      re_patterns
%type <sval>             re_pattern_constant
%type <types>            types
%type <map_element>      map_elem
%type <map_elements>     map_elems opt_map_elems
%type <unit_ctor_item>   unit_ctor_item
%type <unit_ctor_items>  unit_ctor_items opt_unit_ctor_items
%type <ival>             opt_priority
%%

%token PREC_HIGH;

%left OR AND;
%left EQ NEQ LEQ GEQ '<' '>';
%left '+' '-';

// All keywords introducing constants must be listed here before PREC_HIGH.
%left INTERVAL TIME;
%left INT INT8 INT16 INT32 INT64;
%left UINT UINT8 UINT16 UINT32 UINT64;
%left PREC_HIGH;

// Magic states send by the scanner to provide two separate entry points.
%token START_MODULE START_EXPR;
%start start;

start         : START_MODULE module
              | START_EXPR expr
              ;

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

path_id       : IDENT                            { $$ = std::make_shared<ID>($1, loc(@$)); }
              | PATH_IDENT                       { $$ = std::make_shared<ID>($1, loc(@$)); }

hook_id       : scoped_id                        { $$ = $1; }
              | PROPERTY                         { $$ = std::make_shared<ID>($1, loc(@$)); } /* for %init/%done */

dollar_id     : DOLLAR_IDENT                     { $$ = std::make_shared<ID>($1, loc(@$)); }

property      : PROPERTY ';'                     { $$ = std::make_shared<Attribute>($1, nullptr, loc(@$)); }
              | PROPERTY '=' expr ';'            { $$ = std::make_shared<Attribute>($1, $3, loc(@$)); }
              | PROPERTY '=' NONE ';'            { $$ = std::make_shared<Attribute>($1, nullptr, loc(@$)); }
              | PROPERTY '=' base_type ';'       { auto e = std::make_shared<expression::Type>($3, loc(@$));
                                                   $$ = std::make_shared<Attribute>($1, e, loc(@$));
                                                 }

opt_global_decls
              : global_decl opt_global_decls     { $$ = $2; if ( $1 ) $$.push_front($1); }
              | /* empty */                      { $$ = declaration_list(); }
              ;

global_decl   : type_decl                        { $$ = $1; }
              | var_decl                         { $$ = $1; }
              | const_decl                       { $$ = $1; }
              | func_decl                        { $$ = $1; }
              | hook_decl                        { $$ = $1; }

              | property                         { driver.module()->addProperty($1); }
              | import                           { $$ = nullptr; }
              | stmt                             { driver.module()->body()->addStatement($1); }
              ;

var_decl      : opt_linkage GLOBAL local_id type_or_opt_init ';'
                                                 { auto var = std::make_shared<variable::Global>($3, $4.first, $4.second, loc(@$));
                                                   $$ = std::make_shared<declaration::Variable>($3, $1, var, loc(@$));
                                                 }

const_decl    : opt_linkage CONST local_id type_or_init ';'
                                                 { auto type = $4.first;
                                                   auto init = $4.second;

                                                   if ( init->canCoerceTo(type) )
                                                       init = init->coerceTo(type);
                                                   else {
                                                       error(@$, "cannot coerce init expression to type");
                                                       type = init->type();
                                                   }

                                                   $$ = std::make_shared<declaration::Constant>($3, $1, init, loc(@$));
                                                 }

opt_linkage   : EXPORT                           { $$ = Declaration::EXPORTED; }
              | /* empty */ %prec PREC_HIGH      { $$ = Declaration::PRIVATE; }


type_or_init  : ':' base_type init_expr          { $$ = std::make_tuple($2, $3); }
              | init_expr                        { $$ = std::make_tuple($1->type(), $1); }

type_or_opt_init  : ':' type opt_init_expr       { $$ = std::make_tuple($2, $3); }
              | init_expr                        { $$ = std::make_tuple($1->type(), $1); }

opt_init_expr : init_expr                        { $$ = $1; }
              | /* empty */                      { $$ = nullptr; }

init_expr     : '=' expr                         { $$ = $2; }

import        : IMPORT path_id ';'              { driver.module()->import($2); }

type_decl     : opt_linkage TYPE local_id '=' type ';'
                                                 { $$ = std::make_shared<declaration::Type>($3, $1, $5, loc(@$));

                                                   if ( $1 == Declaration::EXPORTED )
                                                       driver.module()->exportType($5);
                                                 }

func_decl     : opt_linkage opt_cc rtype local_id '(' opt_params ')' block
                                                 { auto ftype = std::make_shared<type::Function>($3, $6, $2, loc(@$));
                                                   auto func  = std::make_shared<Function>($4, ftype, driver.module(), $8, loc(@$));
                                                   $$ = std::make_shared<declaration::Function>(func, $1, loc(@$));
                                                 }

              | IMPORT opt_cc rtype scoped_id '(' opt_params ')' opt_hilti_id ';'
                                                 { auto ftype = std::make_shared<type::Function>($3, $6, $2, loc(@$));
                                                   auto func  = std::make_shared<Function>($4, ftype, driver.module(), nullptr, loc(@$));
                                                   $$ = std::make_shared<declaration::Function>(func, Declaration::IMPORTED, loc(@$));
                                                   if ( $8 ) func->setHiltiFunctionID($8);
                                                 }
              ;

hook_decl     : ON hook_id unit_hook             { $$ = std::make_shared<declaration::Hook>($2, $3, loc(@$)); }
              ;

params        : param ',' params                 { $$ = $3; $$.push_front($1); }
              | param                            { $$ = parameter_list(); $$.push_back($1); }

opt_params    : params                           { $$ = $1; }
              | /* empty */                      { $$ = parameter_list(); }

opt_hilti_id  : ATTR_HILTI_ID '=' scoped_id      { $$ = $3; }
              | /* empty */                      { $$ = nullptr; }

param         : opt_param_const opt_param_clear local_id ':' type
                                                 { $$ = std::make_shared<type::function::Parameter>($3, $5, $1, $2, nullptr, loc(@$)); }

rtype         : opt_param_const type             { $$ = std::make_shared<type::function::Result>($2, $1, loc(@$)); }

opt_cc        : CSTRING                          {
                                                   if ( $1 == "HILTI" ) $$ = type::function::HILTI;
                                                   else if ( $1 == "BINPAC-HILTI" ) $$ = type::function::BINPAC_HILTI;
                                                   else if ( $1 == "BINPAC-HILTI-C" ) $$ = type::function::BINPAC_HILTI_C;
                                                   else if ( $1 == "HILTI-C" ) $$ = type::function::HILTI_C;
                                                   else if ( $1 == "C" ) $$ = type::function::C;
                                                   else error(@$, "unknown calling convention");
                                                 }
              | /* empty */                      { $$ = type::function::BINPAC; }

opt_param_const
              : CONST                            { $$ = true; }
              | /* empty */                      { $$ = false; }

opt_param_clear
              : CLEAR                            { $$ = true; }
              | /* empty */                      { $$ = false; }

stmt          : block                            { $$ = $1; }
              | expr ';'                         { $$ = std::make_shared<statement::Expression>($1, loc(@$)); }
              | PRINT exprs ';'                  { $$ = std::make_shared<statement::Print>($2, loc(@$)); }
              | RETURN opt_expr ';'              { $$ = std::make_shared<statement::Return>($2, loc(@$)); }
              | STOP ';'                         { $$ = std::make_shared<statement::Stop>(loc(@$)); }
              | IF '(' expr ')' stmt             { $$ = std::make_shared<statement::IfElse>($3, $5, nullptr, loc(@$)); }
              | IF '(' expr ')' stmt ELSE stmt   { $$ = std::make_shared<statement::IfElse>($3, $5, $7, loc(@$)); }

stmts         : stmt stmts                       { $$ = $2; $$.push_front($1); }
              | stmt                             { $$ = statement_list(); $$.push_back($1); }

opt_stmts     : stmts                            { $$ = $1; }
              | /* empty */                      { $$ = statement_list(); }

block         : '{'                              { driver.pushBlock(std::make_shared<statement::Block>(driver.scope(), loc(@$))); }
                opt_local_decls                  { driver.block()->addDeclarations($3); }
                opt_stmts '}'                    { auto b = driver.popBlock(); b->addStatements($5); $$ = b; }

opt_local_decls
              : local_decl opt_local_decls       { $$ = $2; $$.push_front($1); }
              | /* empty */                      { $$ = declaration_list(); }

local_decl    : LOCAL local_id type_or_opt_init ';'
                                                 { auto v = std::make_shared<variable::Local>($2, $3.first, $3.second, loc(@$));
                                                   $$ = std::make_shared<declaration::Variable>($2, Declaration::PRIVATE, v, loc(@$)); }

type          : base_type                        { $$ = $1; }
              | scoped_id                        { $$ = std::make_shared<type::Unknown>($1, loc(@$)); }

base_type     : atomic_type                      { $$ = $1; }
              | container_type                   { $$ = $1; }

atomic_type   : ANY                              { $$ = std::make_shared<type::Any>(loc(@$)); }
              | ADDR                             { $$ = std::make_shared<type::Address>(loc(@$)); }
              | BOOL                             { $$ = std::make_shared<type::Bool>(loc(@$)); }
              | BYTES                            { $$ = std::make_shared<type::Bytes>(loc(@$)); }
              | CADDR                            { $$ = std::make_shared<type::CAddr>(loc(@$)); }
              | DOUBLE                           { $$ = std::make_shared<type::Double>(loc(@$)); }
              | FILE                             { $$ = std::make_shared<type::File>(loc(@$)); }
              | INTERVAL                         { $$ = std::make_shared<type::Interval>(loc(@$)); }
              | NET                              { $$ = std::make_shared<type::Network>(loc(@$)); }
              | PORT                             { $$ = std::make_shared<type::Port>(loc(@$)); }
              | SINK                             { $$ = std::make_shared<type::Sink>(loc(@$)); }
              | STRING                           { $$ = std::make_shared<type::String>(loc(@$)); }
              | TIME                             { $$ = std::make_shared<type::Time>(loc(@$)); }
              | TIMER                            { $$ = std::make_shared<type::Timer>(loc(@$)); }
              | VOID                             { $$ = std::make_shared<type::Void>(loc(@$)); }

              | INT '<' CINTEGER '>'             { $$ = std::make_shared<type::Integer>($3, true, loc(@$)); }
              | INT8                             { $$ = std::make_shared<type::Integer>(8, true, loc(@$)); }
              | INT16                            { $$ = std::make_shared<type::Integer>(16, true, loc(@$)); }
              | INT32                            { $$ = std::make_shared<type::Integer>(32, true, loc(@$)); }
              | INT64                            { $$ = std::make_shared<type::Integer>(64, true, loc(@$)); }

              | UINT '<' CINTEGER '>'            { $$ = std::make_shared<type::Integer>($3, false, loc(@$)); }
              | UINT8                            { $$ = std::make_shared<type::Integer>(8, false, loc(@$)); }
              | UINT16                           { $$ = std::make_shared<type::Integer>(16, false, loc(@$)); }
              | UINT32                           { $$ = std::make_shared<type::Integer>(32, false, loc(@$)); }
              | UINT64                           { $$ = std::make_shared<type::Integer>(64, false, loc(@$)); }
              | ITER '<' type '>'                { auto iterable = ast::type::checkedTrait<type::trait::Iterable>($3);
                                                   $$ = iterable->iterType();
                                                 }
              | REGEXP                           { $$ = std::make_shared<type::RegExp>(attribute_list(), loc(@$)); }
              | TUPLE '<' types '>'              { $$ = std::make_shared<type::Tuple>($3, loc(@$)); }
              | TUPLE '<' '*' '>'                { $$ = std::make_shared<type::Tuple>(loc(@$)); }

              | OBJECT '<' type '>'              { $$ = std::make_shared<type::EmbeddedObject>($3); }
              | MARK                             { $$ = std::make_shared<type::Mark>(); }

              | OPTIONAL '<' type '>'            { $$ = std::make_shared<type::Optional>($3); }

              | bitfield                         { $$ = $1; }
              | bitset                           { $$ = $1; }
              | enum_                            { $$ = $1; }
              | unit                             { $$ = $1; }
              ;

container_type:
                LIST '<' type '>'                { $$ = std::make_shared<type::List>($3, loc(@$)); }
              | MAP '<' type ',' type '>'        { $$ = std::make_shared<type::Map>($3, $5, loc(@$)); }
              | SET  '<' type '>'                { $$ = std::make_shared<type::Set>($3, loc(@$)); }
              | VECTOR '<' type '>'              { $$ = std::make_shared<type::Vector>($3, loc(@$)); }
              ;

bitset        : BITSET '{' id_with_ints '}'      { $$ = std::make_shared<type::Bitset>($3, loc(@$)); }

enum_         : ENUM '{' id_with_ints '}'        { $$ = std::make_shared<type::Enum>($3, loc(@$)); }
              ;

types         : type ',' types                   { $$ = $3; $$.push_front($1); }
              | type                             { $$ = type_list(); $$.push_back($1); }

id_with_ints  : id_with_ints ',' id_with_int     { $$ = $1; $$.push_back($3); }
              | id_with_int                      { $$ = std::list<std::pair<shared_ptr<ID>, int>>(); $$.push_back($1); }
              ;

id_with_int   : local_id                         { $$ = std::make_pair($1, -1); }
              | local_id '=' CINTEGER            { $$ = std::make_pair($1, $3); }
              ;

bitfield      : BITFIELD '(' CINTEGER ')' '{' bitfield_bits '}'
                                                 { auto itype = std::make_shared<type::Integer>($3, false, loc(@$));
                                                   itype->setBits($6);
                                                   $$ = itype;
                                                 }

bitfield_bits:  bitfield_bits_spec bitfield_bits { $$ = $2; $$.push_front($1); }
              | /* empty */                      { $$ = type::Integer::bits_list(); }

bitfield_bits_spec
              : local_id ':' CINTEGER DOTDOT CINTEGER opt_type_attrs ';'
                                                 { $$ = std::make_shared<type::integer::Bits>($1, $3, $5, $6, loc(@$)); }
              | local_id ':' CINTEGER opt_type_attrs ';'
                                                 { $$ = std::make_shared<type::integer::Bits>($1, $3, $3, $4, loc(@$)); }

opt_unit_params
              : '(' opt_params ')'               { $$ = $2; }
              | /* empty */                      { $$ = parameter_list(); }

unit          : UNIT opt_unit_params '{' opt_unit_items '}'
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

unit_var      : VAR local_id ':' type opt_init_expr opt_unit_hooks
                                                 { $$ = std::make_shared<type::unit::item::Variable>($2, $4, $5, $6, loc(@$)); }

unit_global_hook : ON hook_id unit_hooks         { $$ = std::make_shared<type::unit::item::GlobalHook>($2, $3, loc(@$)); }

unit_prop     : property                         { $$ = std::make_shared<type::unit::item::Property>($1, loc(@$)); }

unit_field    : opt_unit_field_name base_type opt_field_args opt_unit_vector_len opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks
                                                 { $$ = type::unit::item::Field::createByType($2, $1, $6, ($4 ? hook_list() : $8), $5, $3, $7, loc(@$));
                                                   if ( $4 )
                                                       $$ = makeVectorField($$, $1, $4, $8, loc(@$));
                                                 }

              | opt_unit_field_name constant opt_unit_vector_len opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks
                                                 { $$ = std::make_shared<type::unit::item::field::Constant>($1, $2, $5, ($3 ? hook_list() : $7), $4, $6, loc(@$));
                                                   if ( $3 ) $$ = makeVectorField($$, $1, $3, $7, loc(@$));
                                                 }

              | opt_unit_field_name ctor opt_unit_vector_len opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks
                                                 { $$ = std::make_shared<type::unit::item::field::Ctor>($1, $2, $5, ($3 ? hook_list() : $7), $4, $6, loc(@$));
                                                   if ( $3 ) $$ = makeVectorField($$, $1, $3, $7, loc(@$));
                                                 }

              | opt_unit_field_name LIST '<' unit_field_in_container '>' opt_unit_vector_len opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks
                                                 { $$ = std::make_shared<type::unit::item::field::container::List>($1, $4, $8, ($6 ? hook_list() : $10), $7, $9, loc(@$));
                                                  if ( $6 ) $$ = makeVectorField($$, $1, $6, $10, loc(@$));
                                                 }

              | opt_unit_field_name scoped_id opt_field_args opt_unit_vector_len opt_type_attrs opt_unit_field_cond opt_unit_field_sinks opt_unit_hooks
                                                 { $$ = std::make_shared<type::unit::item::field::Unknown>($1, $2, $6, ($4 ? hook_list() : $8), $5, $3, $7, loc(@$));
                                                  if ( $4 ) $$ = makeVectorField($$, $1, $4, $8, loc(@$));
                                                 }

unit_field_in_container :
              local_id opt_field_args          { $$ = std::make_shared<type::unit::item::field::Unknown>(nullptr, $1, nullptr, hook_list(), attribute_list(), $2, expression_list(), loc(@$)); }

unit_fields   : unit_field unit_fields         { $$ = $2; $$.push_front($1); }
              | unit_field                     { $$ = unit_field_list(); $$.push_back($1); }

unit_switch   : SWITCH opt_unit_switch_expr '{' unit_switch_cases '}' opt_unit_field_cond ';'
                                                 { $$ = std::make_shared<type::unit::item::field::Switch>($2, $4, $6, hook_list(), loc(@$)); }

opt_unit_switch_expr: '(' expr ')'               { $$ = $2; }
              | /* empty */                      { $$ = nullptr; }

unit_switch_cases
              : unit_switch_case unit_switch_cases
                                                 { $$ = $2; $$.push_front($1); }
              | unit_switch_case                 { $$ = type::unit::item::field::Switch::case_list(); $$.push_back($1); }

unit_switch_case
              : exprs ARROW '{' unit_fields '}'  { $$ = std::make_shared<type::unit::item::field::switch_::Case>($1, $4, loc(@$)); }
              | '*'   ARROW '{' unit_fields '}'  { $$ = std::make_shared<type::unit::item::field::switch_::Case>($4, loc(@$)); }
              | exprs ARROW unit_field           { $$ = std::make_shared<type::unit::item::field::switch_::Case>($1, $3, loc(@$)); }
              | '*'   ARROW unit_field           { $$ = std::make_shared<type::unit::item::field::switch_::Case>($3, loc(@$)); }
              | unit_field                       { $$ = std::make_shared<type::unit::item::field::switch_::Case>(expression_list(), $1, loc(@$)); }


opt_type_attrs: type_attr opt_type_attrs         { $$ = $2; $$.push_front($1); }
              | /* empty */                      { $$ = attribute_list(); }

type_attr     : ATTRIBUTE '(' expr ')'           { $$ = std::make_shared<Attribute>($1, $3, loc(@$)); }
              | ATTRIBUTE '=' expr               { $$ = std::make_shared<Attribute>($1, $3, loc(@$)); }
              | ATTRIBUTE                        { $$ = std::make_shared<Attribute>($1, nullptr, loc(@$)); }

opt_field_args: '(' opt_exprs ')'                { $$ = $2; }
              | /* empty */                      { $$ = expression_list(); }

opt_unit_field_name
              : local_id ':'                     { $$ = $1; }
              | ':'                              { $$ = nullptr; }

opt_unit_field_cond
              : IF '(' expr ')'                  { $$ = $3; }
              | /* empty */                      { $$ = nullptr; }

opt_unit_field_sinks
              : ARROW exprs                      { $$ = $2; }
              | /* empty */                      { $$ = expression_list(); }

opt_unit_hooks: unit_hooks                       { $$ = $1; }
              | ';'                              { $$ = hook_list(); }

opt_unit_vector_len
              : '[' expr ']'                     { $$ = $2; }
              | /* empty */                      { $$ = nullptr; }

unit_hooks    : unit_hook unit_hooks             { $$ = $2; $2.push_front($1); }
              | unit_hook                        { $$ = { $1 }; }

unit_hook     : opt_debug opt_priority opt_foreach
                                                 { driver.pushScope(std::make_shared<Scope>(driver.module()->body()->scope())); }
                block                            { $$ = std::make_shared<Hook>($5, $2, $1, $3, loc(@$)); driver.popScope(); }

opt_debug     : PROPERTY                         { $$ = ($1 == "%debug");
                                                   if ( ! $$ ) error(@$, "unexpected property, only %debug permitted");
                                                 }

              | /* empty */                      { $$ = false; }


opt_priority  : PRIORITY '=' CINTEGER            { $$ = $3; };

              | /* empty */                      { $$ = 0; }

opt_foreach   : FOREACH                          { $$ = true; }
              | /* empty */                      { $$ = false; }

              /* When adding rules here, add leading keywords to the precedence declaration above. */
constant      : CINTEGER                         { $$ = std::make_shared<constant::Integer>($1, 64, loc(@$)); }
              | UINT '<' CINTEGER '>' '(' CINTEGER ')'
                                                 { $$ = std::make_shared<constant::Integer>($6, $3, false, loc(@$)); }
              | UINT8 '(' CINTEGER ')'           { $$ = std::make_shared<constant::Integer>($3, 8, false, loc(@$)); }
              | UINT16 '(' CINTEGER ')'          { $$ = std::make_shared<constant::Integer>($3, 16, false, loc(@$)); }
              | UINT32 '(' CINTEGER ')'          { $$ = std::make_shared<constant::Integer>($3, 32, false, loc(@$)); }
              | UINT64 '(' CINTEGER ')'          { $$ = std::make_shared<constant::Integer>($3, 64, false, loc(@$)); }

              | INT '<' CINTEGER '>' '(' CINTEGER ')'
                                                 { $$ = std::make_shared<constant::Integer>($6, $3, true, loc(@$)); }
              | INT8 '(' CINTEGER ')'            { $$ = std::make_shared<constant::Integer>($3, 8, true, loc(@$)); }
              | INT16 '(' CINTEGER ')'           { $$ = std::make_shared<constant::Integer>($3, 16, true, loc(@$)); }
              | INT32 '(' CINTEGER ')'           { $$ = std::make_shared<constant::Integer>($3, 32, true, loc(@$)); }
              | INT64 '(' CINTEGER ')'           { $$ = std::make_shared<constant::Integer>($3, 64, true, loc(@$)); }

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
              | tuple                            { $$ = $1; }
              | optional                         { $$ = $1; }
              ;

tuple         : '(' opt_exprs ')'                { $$ = std::make_shared<constant::Tuple>($2, loc(@$)); }
              ;

optional      : OPTIONAL '(' expr ')'            { $$ = std::make_shared<constant::Optional>($3, loc(@$)); }
              | OPTIONAL '(' ')'                 { $$ = std::make_shared<constant::Optional>(loc(@$)); }
              ;

ctor          : CBYTES                           { $$ = std::make_shared<ctor::Bytes>($1, loc(@$)); }
              | re_patterns                      { $$ = std::make_shared<ctor::RegExp>($1, attribute_list(), loc(@$)); }

              | LIST                '(' opt_exprs ')' { $$ = std::make_shared<ctor::List>(nullptr, $3, loc(@$)); }
//            |                     '[' opt_exprs ']' { $$ = std::make_shared<ctor::List>(nullptr, $2, loc(@$)); }
              | LIST   '<' type '>' '(' opt_exprs ')' { $$ = std::make_shared<ctor::List>($3, $6, loc(@$)); }

              | SET                 '(' opt_exprs ')' { $$ = std::make_shared<ctor::Set>(nullptr, $3, loc(@$)); }
              | SET    '<' type '>' '(' opt_exprs ')' { $$ = std::make_shared<ctor::Set>($3, $6, loc(@$)); }

              | MAP                          '(' opt_map_elems ')' { $$ = std::make_shared<ctor::Map>(nullptr, nullptr, $3, loc(@$)); }
              | MAP    '<' type ',' type '>' '(' opt_map_elems ')' { $$ = std::make_shared<ctor::Map>($3, $5, $8, loc(@$)); }

              | VECTOR              '(' opt_exprs ')' { $$ = std::make_shared<ctor::Vector>(nullptr, $3, loc(@$)); }
              | VECTOR '<' type '>' '(' opt_exprs ')' { $$ = std::make_shared<ctor::Vector>($3, $6, loc(@$)); }

              | UNIT                '(' opt_unit_ctor_items ')'
                                                 { $$ = std::make_shared<ctor::Unit>($3, loc(@$)); }
              ;

opt_map_elems : map_elems                        { $$ = $1; }
              | /* empty */                      { $$ = ctor::Map::element_list(); }

map_elems     : map_elems ',' map_elem           { $$ = $1; $$.push_back($3); }
              | map_elem                         { $$ = ctor::Map::element_list(); $$.push_back($1); }

map_elem      : expr ':' expr                    { $$ = ctor::Map::element($1, $3); }

opt_unit_ctor_items :
                unit_ctor_items                  { $$ = $1; }
              | /* empty */                      { $$ = ctor::Unit::item_list(); }

unit_ctor_items
              : unit_ctor_items ',' unit_ctor_item  { $$ = $1; $$.push_back($3); }
              | unit_ctor_item                      { $$ = ctor::Unit::item_list(); $$.push_back($1); }

unit_ctor_item
              : dollar_id '=' expr               { $$ = ctor::Unit::item($1, $3); }
              | expr                             { $$ = ctor::Unit::item(nullptr, $1); }

re_patterns   : re_patterns '|' re_pattern_constant { $$ = $1; $$.push_back($3); }
              | re_pattern_constant                 { $$ = ctor::RegExp::pattern_list(); $$.push_back($1); }

re_pattern_constant
              : '/' { driver.enablePatternMode(); } CREGEXP { driver.disablePatternMode(); } '/' { $$ = $3; }

expr          : expr2                            { driver.setExpression($1); }

expr2         : scoped_id                        { $$ = std::make_shared<expression::ID>($1, loc(@$)); }
              | '(' expr ')'                     { $$ = $2; }
              | ctor                             { $$ = std::make_shared<expression::Ctor>($1, loc(@$)); }
              | constant                         { $$ = std::make_shared<expression::Constant>($1, loc(@$)); }
              | expr '=' expr                    { $$ = std::make_shared<expression::Assign>($1, $3, loc(@$)); }
              | expr '?' expr ':' expr           { $$ = std::make_shared<expression::Conditional>($1, $3, $5, loc(@$)); }
              | '[' expr FOR local_id IN expr ']'{ $$ = std::make_shared<expression::ListComprehension>($2, $4, $6, nullptr, loc(@$)); }
              | '[' expr FOR local_id IN expr IF expr ']'
                                                 { $$ = std::make_shared<expression::ListComprehension>($2, $4, $6, $8, loc(@$)); }


/*            | atomic_type '(' ')'                { $$ = std::make_shared<expression::Default>($1, loc(@$)); } */

              /* Overloaded operators */

              | expr tuple_expr                  { $$ = makeOp(operator_::Call, { $1, $2 }, loc(@$)); }
              | expr '[' expr ']'                { $$ = makeOp(operator_::Index, { $1, $3 }, loc(@$)); }
              | expr AND expr                    { $$ = makeOp(operator_::LogicalAnd, { $1, $3 }, loc(@$)); }
              | expr OR expr                     { $$ = makeOp(operator_::LogicalOr, { $1, $3 }, loc(@$)); }
              | expr '&' expr                    { $$ = makeOp(operator_::BitAnd, { $1, $3 }, loc(@$)); }
              | expr '|' expr                    { $$ = makeOp(operator_::BitOr, { $1, $3 }, loc(@$)); }
              | expr '^' expr                    { $$ = makeOp(operator_::BitXor, { $1, $3 }, loc(@$)); }
              | expr SHIFTLEFT expr              { $$ = makeOp(operator_::ShiftLeft, { $1, $3 }, loc(@$)); }
              | expr SHIFTRIGHT expr             { $$ = makeOp(operator_::ShiftRight, { $1, $3 }, loc(@$)); }
              | expr POW expr                    { $$ = makeOp(operator_::Power, { $1, $3 }, loc(@$)); }
              | expr '.' member_expr             { $$ = makeOp(operator_::Attribute, { $1, $3 }, loc(@$)); }
              | expr '.' member_expr '=' expr    { $$ = makeOp(operator_::AttributeAssign, { $1, $3, $5 }, loc(@$)); }
              | expr HASATTR member_expr         { $$ = makeOp(operator_::HasAttribute, { $1, $3 }, loc(@$)); }
              | expr TRYATTR member_expr         { $$ = makeOp(operator_::TryAttribute, { $1, $3 }, loc(@$)); }
              | expr PLUSPLUS                    { $$ = makeOp(operator_::IncrPostfix, { $1 }, loc(@$)); }
              | PLUSPLUS expr                    { $$ = makeOp(operator_::IncrPrefix, { $2 }, loc(@$)); }
              | expr MINUSMINUS                  { $$ = makeOp(operator_::DecrPostfix, { $1 }, loc(@$)); }
              | MINUSMINUS expr                  { $$ = makeOp(operator_::DecrPrefix, { $2 }, loc(@$)); }
              | '*' expr                         { $$ = makeOp(operator_::Deref, { $2 }, loc(@$)); }
              | '!' expr                         { $$ = makeOp(operator_::Not, { $2 }, loc(@$)); }
              | '|' expr '|'                     { $$ = makeOp(operator_::Size, { $2 }, loc(@$)); }
              | expr '+' expr                    { $$ = makeOp(operator_::Plus, { $1, $3 }, loc(@$)); }
              | expr '-' expr                    { $$ = makeOp(operator_::Minus, { $1, $3 }, loc(@$)); }
              | expr '*' expr                    { $$ = makeOp(operator_::Mult, { $1, $3 }, loc(@$)); }
              | expr '/' expr                    { $$ = makeOp(operator_::Div, { $1, $3 }, loc(@$)); }
              | expr MOD expr                    { $$ = makeOp(operator_::Mod, { $1, $3 }, loc(@$)); }
              | expr EQ expr                     { $$ = makeOp(operator_::Equal, { $1, $3 }, loc(@$)); }
              | expr IN expr                     { $$ = makeOp(operator_::In, { $1, $3 }, loc(@$)); }
              | expr '<' expr                    { $$ = makeOp(operator_::Lower, { $1, $3 }, loc(@$)); }
              | expr PLUSASSIGN expr             { $$ = makeOp(operator_::PlusAssign, { $1, $3 }, loc(@$)); }
              | expr MINUSASSIGN expr            { $$ = makeOp(operator_::MinusAssign, { $1, $3 }, loc(@$)); }
              | expr '[' expr ']' '=' expr       { $$ = makeOp(operator_::IndexAssign, { $1, $3, $6 }, loc(@$)); }
              | expr '.' member_expr tuple_expr  { $$ = makeOp(operator_::MethodCall, { $1, $3, $4 }, loc(@$)); }
              | ADD expr '[' expr ']'            { $$ = makeOp(operator_::Add, { $2, $4 }, loc(@$)); }
              | DELETE expr '[' expr ']'         { $$ = makeOp(operator_::Delete, { $2, $4 }, loc(@$)); }
              | CAST '<' type '>' '(' expr ')'   { $$ = makeOp(operator_::Cast, { $6, std::make_shared<expression::Type>($3) }, loc(@$)); }
              | NEW id_expr                      { $$ = makeOp(operator_::New, {$2 }, loc(@$)); }
              | NEW id_expr tuple_expr           { $$ = makeOp(operator_::New, {$2, $3}, loc(@$)); }

              /* Operators derived from other operators. */

              | expr NEQ expr                    { auto e = makeOp(operator_::Equal, { $1, $3 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e }, loc(@$));
                                                 }

              | expr LEQ expr                    { auto e = makeOp(operator_::Greater, { $1, $3 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e }, loc(@$));
                                                 }

              | expr GEQ expr                    { auto e = makeOp(operator_::Lower, { $1, $3 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e }, loc(@$));
                                                 }

              | expr '>' expr                    { auto e1 = makeOp(operator_::Lower, { $1, $3 }, loc(@$));
                                                   auto e2 = makeOp(operator_::Equal, { $1, $3 }, loc(@$));
                                                   auto e3 = makeOp(operator_::LogicalOr, { e1, e2 }, loc(@$));
                                                   $$ = makeOp(operator_::Not, { e3 }, loc(@$));
                                                 }

opt_expr      : expr                             { $$ = $1; }
              | /* empty */                      { $$ = nullptr; }

id_expr       : scoped_id                        { $$ = std::make_shared<expression::ID>($1, loc(@$)); }

member_expr   : local_id                         { $$ = std::make_shared<expression::MemberAttribute>($1, loc(@$)); }

tuple_expr    : tuple                            { $$ = std::make_shared<expression::Constant>($1, loc(@$)); }

exprs         : expr ',' exprs                   { $$ = $3; $$.push_front($1); }
              | expr                             { $$ = expression_list(); $$.push_back($1); }

opt_exprs     : exprs                            { $$ = $1; }
              | /* empty */                      { $$ = expression_list(); }

%%

void binpac_parser::Parser::error(const Parser::location_type& l, const std::string& m)
{
    driver.error(m, l);
}
