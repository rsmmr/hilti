
// This borrows heavily from https://idlebox.net/2007/flex-bison-cpp-example.

#ifndef SPICY_SCANNER_H
#define SPICY_SCANNER_H

#include <string>

#include "driver.h"
#include <spicy/autogen/parser.h>

namespace spicy_parser {

class Scanner : public SpicyFlexLexer {
public:
    Scanner(std::istream* yyin = 0, std::ostream* yyout = 0) : SpicyFlexLexer(yyin, yyout)
    {
    }
    virtual Parser::token_type lex(Parser::semantic_type* yylval, Parser::location_type* yylloc,
                                   spicy_parser::Driver& driver);

    void disablePatternMode();
    void enablePatternMode();
};
}

#endif
