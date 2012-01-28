
%{
// Disable: scanner.ll:41:1: warning: expression result unused [-Wunused-value]
#pragma clang diagnostic ignored "-Wunused-value"

#include <string>

#include "scanner.h"
#include "driver.h"

#define yyterminate() return token::END

%}

%option c++
%option prefix="Hilti"
%option noyywrap nounput batch debug yylineno

%{
#define YY_USER_ACTION yylloc->columns(yyleng);
%}

id      @?[a-zA-Z_][a-zA-Z_0-9]*
string  \"(\\.|[^\\"])*\"
int     [+-]?[0-9]+
blank   [ \t]
comment [ \t]*#[^\n]*

%%
%{
    yylloc->step ();
%}

%{
    typedef hilti_parser::Parser::token token;
    typedef hilti_parser::Parser::token_type token_type;
%}

{blank}+              yylloc->step();
[\n]+                 yylloc->lines(yyleng); return token::NEWLINE;

\"C-HILTI\"           yylval->sval = string(yytext, 1, strlen(yytext) - 2); return token::CSTRING;
\"C\"                 yylval->sval = string(yytext, 1, strlen(yytext) - 2); return token::CSTRING;

module                return token::MODULE;
global                return token::GLOBAL;
local                 return token::LOCAL;
import                return token::IMPORT;
const                 return token::CONST;
declare               return token::DECLARE;
export                return token::EXPORT;

void                  return token::VOID;
any                   return token::ANY;
string                return token::STRING;
int                   return token::INT;
bool                  return token::BOOL;
tuple                 return token::TUPLE;
ref                   return token::REF;
iter                  return token::ITER;
bytes                 return token::BYTES;

True                  yylval->bval = 1; return token::CBOOL;
False                 yylval->bval = 0; return token::CBOOL;
null                  return token::CNULL;

{int}                 yylval->ival = atoi(yytext); return token::CINTEGER;
{string}              yylval->sval = string(yytext, 1, strlen(yytext) - 2); return token::CSTRING;
b{string}             yylval->sval = string(yytext, 2, strlen(yytext) - 3); return token::CBYTES;

{id}                  yylval->sval = yytext; return token::IDENT;
{id}(\.{id}){1,}      yylval->sval = yytext; return token::DOTTED_IDENT;
{id}(::{id}){1,}      yylval->sval = yytext; return token::SCOPED_IDENT;


{comment}             yylval->sval = yytext; return token::COMMENT;

[,=;<>(){}]           return (token_type) yytext[0];

.                     driver.error("invalid character", *yylloc);

%%

int HiltiFlexLexer::yylex()
{
    assert(false); // Shouldn't be called.
    return 0;
}
