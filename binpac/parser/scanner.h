
// This borrows heavily from https://idlebox.net/2007/flex-bison-cpp-example.

#ifndef BINPAC_SCANNER_H
#define BINPAC_SCANNER_H

#include <string>

#include "driver.h"
#include <binpac/autogen/parser.h>

namespace binpac_parser {

class Scanner : public BinPACFlexLexer {
public:
    Scanner(std::istream* yyin = 0, std::ostream* yyout = 0) : BinPACFlexLexer(yyin, yyout)
    {
    }
    virtual Parser::token_type lex(Parser::semantic_type* yylval, Parser::location_type* yylloc,
                                   binpac_parser::Driver& driver);

    void disablePatternMode();
    void enablePatternMode();
};
}

#endif
