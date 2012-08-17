//
// @TEST-EXEC: ${SCRIPTS}/build-with-binpac -I %DIR %INPUT -o a.out
// @TEST-EXEC: ./a.out >out
// @TEST-EXEC: btest-diff out
//

#include <grammar.h>
#include <constant.h>
#include <expression.h>

#include "production-helpers.cc"

int main(int argc, char** argv)
{
    // Simple example grammar from
    //
    // http://www.cs.uky.edu/~lewis/essays/compilers/td-parse.html
    //
    // Ambiguity expected.

    auto a = literal("a", "a");
    auto b = literal("b", "b");
    auto c = literal("c", "c");
    auto d = literal("d", "d");

    auto cC = sequence("cC", {});
    auto bD = sequence("bD", {});
    auto AD = sequence("AD", {});
    auto aA = sequence("aA", {});

    auto A = lookAhead("A", epsilon(), a);
    auto B = lookAhead("B", epsilon(), bD);
    auto C = lookAhead("C", AD, b);
    auto D = lookAhead("D", aA, c);

    auto ABA = sequence("ABA", { A, B, A });
    auto S = lookAhead("S", ABA, cC);

    cC->add(c);
    cC->add(C);

    bD->add(b);
    bD->add(D);

    AD->add(A);
    AD->add(D);

    aA->add(a);
    aA->add(A);

    auto g = std::make_shared<binpac::Grammar>("my-grammar", S);
    g->printTables(std::cout, true);

    auto error = g->check();

    if ( error.size() )
        std::cout << "Grammar error: " << error << std::endl;
}
