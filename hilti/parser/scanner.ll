
%{
// Disable: scanner.ll:41:1: warning: expression result unused [-Wunused-value]
#pragma clang diagnostic ignored "-Wunused-value"

#include <string>

#include "parser/scanner.h"
#include "parser/driver.h"

#define yyterminate() return token::END

%}

%option c++
%option prefix="Hilti"
%option noyywrap nounput batch debug yylineno

%x RE
%s IGNORE_NL

%{
#define YY_USER_ACTION yylloc->columns(yyleng);
%}

id      [a-zA-Z_][a-zA-Z_0-9]*
string  \"(\\.|[^\\"])*\"
int     [+-]?[0-9]+
blank   [ \t]
comment [ \t]*#[^\n]*
digits  [0-9]+
hexs    [0-9a-fA-F]+
address ("::")?({digits}"."){3}{digits}|({hexs}:){7}{hexs}|0x{hexs}({hexs}|:)*"::"({hexs}|:)*|(({digits}|:)({hexs}|:)*)?"::"({hexs}|:)*

%%
%{
    yylloc->step ();
%}

%{
    typedef hilti_parser::Parser::token token;
    typedef hilti_parser::Parser::token_type token_type;
%}

{blank}+              yylloc->step();

<INITIAL>[\n]+        yylloc->lines(yyleng); return token::NEWLINE;
<INITIAL>{comment}    yylval->sval = yytext; return token::COMMENT;

<IGNORE_NL>[\n]+      yylloc->lines(yyleng);
<IGNORE_NL>{comment}  /* Eat in non-newline mode. */

<RE>(\\.|[^\\\/])*    yylval->sval = yytext; return token::CREGEXP;
<RE>[/\\\n]	          return (token_type) yytext[0];

\"C-HILTI\"           yylval->sval = string(yytext, 1, strlen(yytext) - 2); return token::CSTRING;
\"C\"                 yylval->sval = string(yytext, 1, strlen(yytext) - 2); return token::CSTRING;

addr                  return token::ADDR;
any                   return token::ANY;
bitset                return token::BITSET;
bool                  return token::BOOL;
bytes                 return token::BYTES;
caddr                 return token::CADDR;
callable              return token::CALLABLE;
catch                 return token::CATCH;
channel               return token::CHANNEL;
classifier            return token::CLASSIFIER;
const                 return token::CONST;
declare               return token::DECLARE;
double                return token::DOUBLE;
enum                  return token::ENUM;
exception             return token::EXCEPTION;
export                return token::EXPORT;
file                  return token::FILE;
global                return token::GLOBAL;
hook                  return token::HOOK;
import                return token::IMPORT;
int                   return token::INT;
interval              return token::INTERVAL;
iosrc                 return token::IOSRC;
iterator              return token::ITER;
list                  return token::LIST;
local                 return token::LOCAL;
map                   return token::MAP;
match_token_state     return token::MATCH_TOKEN_STATE;
module                return token::MODULE;
net                   return token::NET;
overlay               return token::OVERLAY;
port                  return token::PORT;
ref                   return token::REF;
regexp                return token::REGEXP;
set                   return token::SET;
string                return token::STRING;
struct                return token::STRUCT;
time                  return token::TIME;
timer                 return token::TIMER;
timer_mgr             return token::TIMERMGR;
try                   return token::TRY;
tuple                 return token::TUPLE;
type                  return token::TYPE;
vector                return token::VECTOR;
void                  return token::VOID;
with                  return token::WITH;
after                 return token::AFTER;
at                    return token::AT;
for                   return token::FOR;
in                    return token::IN;
init                  return token::INIT;

False                 yylval->bval = 0; return token::CBOOL;
Null                  return token::CNULL;
True                  yylval->bval = 1; return token::CBOOL;

&default              yylval->sval = yytext; return token::ATTR_DEFAULT;
&group                yylval->sval = yytext; return token::ATTR_GROUP;
&libhilti             yylval->sval = yytext; return token::ATTR_LIBHILTI;
&nosub                yylval->sval = yytext; return token::ATTR_NOSUB;
&priority             yylval->sval = yytext; return token::ATTR_PRIORITY;
&scope                yylval->sval = yytext; return token::ATTR_SCOPE;

{digits}\/(tcp|udp)   yylval->sval = yytext; return token::CPORT;
{address}             yylval->sval = yytext; return token::CADDRESS;

[-+]?{digits}\.{digits} yylval->dval = strtod(yytext, 0); return token::CDOUBLE;

{int}                 yylval->ival = strtoll(yytext, 0, 10); return token::CINTEGER;
{string}              yylval->sval = util::expandEscapes(string(yytext, 1, strlen(yytext) - 2)); return token::CSTRING;
b{string}             yylval->sval = util::expandEscapes(string(yytext, 2, strlen(yytext) - 3)); return token::CBYTES;

{id}                  yylval->sval = yytext; return token::IDENT;
@{id}                 yylval->sval = yytext; return token::LABEL_IDENT;
{id}(\.{id}){1,}      yylval->sval = yytext; return token::DOTTED_IDENT;
{id}(::{id}){1,}      yylval->sval = yytext; return token::SCOPED_IDENT;

[*,=:;<>(){}/|]       return (token_type) yytext[0];

.                     driver.error("invalid character", *yylloc);

%%

int HiltiFlexLexer::yylex()
{
    assert(false); // Shouldn't be called.
    return 0;
}

void hilti_parser::Scanner::disableLineMode()
{
    yy_push_state(IGNORE_NL);
}

void hilti_parser::Scanner::enableLineMode()
{
    yy_pop_state();
}

void hilti_parser::Scanner::enablePatternMode()
{
    yy_push_state(RE);
}

void hilti_parser::Scanner::disablePatternMode()
{
    yy_pop_state();
}
