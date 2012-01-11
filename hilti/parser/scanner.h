
// This borrows heavily from https://idlebox.net/2007/flex-bison-cpp-example.

#ifndef HILTI_SCANNER_H
#define HILTI_SCANNER_H

#include <string>

#include "parser.h"

namespace hilti_parser {

class Scanner : public HiltiFlexLexer
{
public:
   Scanner(std::istream* yyin = 0, std::ostream* yyout = 0) : HiltiFlexLexer(yyin, yyout) {}
   virtual Parser::token_type lex(Parser::semantic_type* yylval, Parser::location_type* yylloc, hilti_parser::Driver& driver);
};

}

#endif
