%skeleton "lalr1.cc"                          /*  -*- C++ -*- */
%require "2.3"
%defines
%define "parser_class_name" "Parser"

%{
namespace hilti_parser { class Parser; }

#include "driver.h"

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

%union {}

%{
#include "scanner.h"
#include "../builder.h"

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
%token <sval>  IDENT          "identifier"
%token <sval>  DOTTED_IDENT   "dotted identifier"
%token <sval>  SCOPED_IDENT   "scoped identifier"
%token <sval>  COMMENT        "comment"

%token <ival>  CINTEGER       "integer"
%token <bval>  CBOOL          "bool"
%token <sval>  CSTRING        "string"
%token <sval>  CBYTES         "bytes"
%token         CNULL          "'null'"

%token         GLOBAL         "'global'"
%token         LOCAL          "'local'"
%token         ANY            "'any'"
%token         VOID           "'void'"
%token         INT            "'int'"
%token         BOOL           "'bool'"
%token         MODULE         "'module'"
%token         STRING         "'string'"
%token         IMPORT         "'import'"
%token         CONST          "'const'"
%token         DECLARE        "'declare'"
%token         EXPORT         "'export'"
%token         TUPLE          "'tuple'"
%token         REF            "'ref'"
%token         ITER           "'iter'"
%token         BYTES          "'bytes'"

%token         NEWLINE        "newline"

%type <decl>             global local function
%type <decls>            opt_local_decls
%type <type>             type
%type <id>               local_id scoped_id mnemonic import
%type <expr>             expr opt_default_expr tuple_elem constant ctor
%type <exprs>            opt_tuple_elem_list tuple_elem_list tuple
%type <types>            type_list
%type <stmt>             stmt instruction
%type <stmts>            stmt_list opt_stmt_list
%type <block>            body
%type <operands>         operands
%type <params>           param_list opt_param_list
%type <param>            param result
%type <bval>             opt_tok_const
%type <sval>             comment opt_comment
%type <cc>               opt_cc

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

opt_nl        : NEWLINE
              | /* empty */
              ;

input         : opt_comment module global_decls
              ;

module        : MODULE local_id eol              { driver.context()->module = builder::module::create($2, *driver.streamName(), loc(@$)); }
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
              | import                           { driver.context()->module->import($1); }
              | export_                          { }
              | comment                          { }
              ;

opt_local_decls : local opt_local_decls          { $$ = $2; $$.push_front($1); }
              | /* empty */                      { $$ = statement::Block::decl_list(); }
              ;

global        : GLOBAL type local_id eol          { $$ = builder::global::create($3, $2, 0, loc(@$)); }
              | GLOBAL type local_id '=' expr eol { $$ = builder::global::create($3, $2, $5, loc(@$)); }
              ;

local         : LOCAL type local_id eol          { $$ = builder::local::create($3, $2, 0, loc(@$)); }
              | LOCAL type local_id '=' expr eol { $$ = builder::local::create($3, $2, $5, loc(@$)); }
              ;

stmt          : instruction eol                  { $$ = $1; }
              | local                            { $$ = $1; }
              ;

opt_stmt_list : stmt_list                        { $$ = $1; }
              | /* empty */                      { $$ = builder::function::statement_list(); }

stmt_list     : stmt stmt_list                   { $$ = $2; $$.push_front($1); }
              | comment stmt_list                { $$ = $2; }
              | stmt                             { $$ = builder::function::statement_list(); $$.push_back($1); }
              | comment                          { $$ = builder::function::statement_list(); }
              ;

instruction   : mnemonic operands                { $$ = builder::instruction::create($1, $2, loc(@$)); }
              | expr '=' mnemonic operands       { $4[0] = $1; $$ = builder::instruction::create($3, $4, loc(@$)); }
              | expr '=' operands                { $3[0] = $1; $$ = builder::instruction::create("assign", $3, loc(@$)); }

mnemonic      : local_id                         { $$ = $1; }
              | DOTTED_IDENT                     { $$ = builder::id::create($1, loc(@$)); }

operands      : expr                             { $$ = make_ops(nullptr, $1, nullptr, nullptr); }
              | expr expr                        { $$ = make_ops(nullptr, $1, $2, nullptr); }
              | expr expr expr                   { $$ = make_ops(nullptr, $1, $2, $3); }

type          : ANY                              { $$ = builder::any::type(loc(@$)); }
              | VOID                             { $$ = builder::void_::type(loc(@$)); }
              | STRING                           { $$ = builder::string::type(loc(@$)); }
              | BOOL                             { $$ = builder::boolean::type(loc(@$)); }
              | BYTES                            { $$ = builder::bytes::type(loc(@$)); }
              | INT '<' CINTEGER '>'             { $$ = builder::integer::type($3, loc(@$)); }
              | TUPLE '<' type_list '>'          { $$ = builder::tuple::type($3, loc(@$)); }
              | TUPLE '<' '*' '>'                { $$ = builder::tuple::typeAny(loc(@$)); }
              | REF '<' type '>'                 { $$ = builder::reference::type($3, loc(@$)); }
              | REF '<' '*' '>'                  { $$ = builder::reference::typeAny(loc(@$)); }
              | ITER '<' type '>'                { $$ = builder::iterator::type($3, loc(@$)); }
              | ITER '<' '*' '>'                 { $$ = builder::iterator::typeAny(loc(@$)); }
              ;

type_list     : type ',' type_list               { $$ = $3; $$.push_front($1); }
              | type                             { $$ = builder::type_list(); $$.push_back($1); }

expr          : constant                         { $$ = $1; }
              | ctor                             { $$ = $1; }
              | scoped_id                        { $$ = builder::id::create($1, loc(@$)); }
              ;

constant      : CINTEGER                         { $$ = builder::integer::create($1, loc(@$)); }
              | CBOOL                            { $$ = builder::boolean::create($1, loc(@$)); }
              | CSTRING                          { $$ = builder::string::create($1, loc(@$)); }
              | CNULL                            { $$ = builder::reference::createNull(loc(@$)); }
              | tuple                            { $$ = builder::tuple::create($1, loc(@$));  }
              ;

ctor          : CBYTES                           { $$ = builder::bytes::create($1, loc(@$)); }
              ;

import        : IMPORT local_id eol              { $$ = $2; }
              ;

export_       : EXPORT local_id eol              { driver.context()->module->exportID($2); }
              | EXPORT type eol                  { driver.context()->module->exportType($2); }
              ;

function      : opt_cc result local_id '(' opt_param_list ')' body        { $$ = builder::function::create($3, $2, $5, driver.context()->module, $1, $7, loc(@$)); }
              | DECLARE opt_cc result local_id '(' opt_param_list ')' eol { $$ = builder::function::create($4, $3, $6, driver.context()->module, $2, nullptr, loc(@$));
                                                                            driver.context()->module->exportID($4);
                                                                          }
              ;

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

body          : opt_nl '{' opt_nl opt_local_decls opt_stmt_list opt_nl '}' opt_nl  { $$ = builder::function::body($4, $5, driver.context()->module, loc(@$)); }

tuple         : '(' opt_tuple_elem_list ')'      { $$ = $2; }

opt_tuple_elem_list : tuple_elem_list            { $$ = $1; }
                    | /* empty */                { $$ = builder::tuple::element_list(); }

tuple_elem_list : tuple_elem ',' tuple_elem_list { $$ = $3; $$.push_front($1); }
                | tuple_elem                     { $$ = builder::tuple::element_list(); $$.push_back($1); }

tuple_elem    : expr                             { $$ = $1; }
              | '*'                              { $$ = builder::unset::create(); }

%%

void hilti_parser::Parser::error(const Parser::location_type& l, const std::string& m)
{
    driver.error(m, l);
}
