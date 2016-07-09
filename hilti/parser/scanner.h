
// This borrows heavily from https://idlebox.net/2007/flex-bison-cpp-example.

#ifndef HILTI_SCANNER_H
#define HILTI_SCANNER_H

#include <string>

#include "driver.h"
#include "hilti/autogen/parser.h"

namespace hilti_parser {

class Scanner : public HiltiFlexLexer {
public:
    Scanner(std::istream* yyin = 0, std::ostream* yyout = 0) : HiltiFlexLexer(yyin, yyout)
    {
    }
    virtual Parser::token_type lex(Parser::semantic_type* yylval, Parser::location_type* yylloc,
                                   hilti_parser::Driver& driver);

    void disableLineMode();
    void enableLineMode();
    void disablePatternMode();
    void enablePatternMode();
};
}

#endif
