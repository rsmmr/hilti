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
    //  Simple example grammar from "Parsing Techniques", Fig. 8.9

    auto hs = literal("hs", "#");
    auto pl = literal("pl", "(");
    auto pr = literal("pr", ")");
    auto no = literal("no", "!");
    auto qu = literal("qu", "?");
    auto st = ::variable("st", std::make_shared<binpac::type::Bytes>());

    auto F = sequence("Fact", {no, st});
    auto Q = sequence("Question", {qu, st});
    auto S = lookAhead("Session", nullptr, nullptr);
    auto SS = sequence("SS", {pl, S, pr, S});
    auto Fs = lookAhead("Facts", nullptr, nullptr);
    auto FsQ = sequence("FsQ", {Fs, Q});
    auto FFs = sequence("FFs", {F, Fs});

    S->setAlternatives(FsQ, SS);
    Fs->setAlternatives(FFs, epsilon());

    auto root = sequence("Start", {S,hs});

    auto g = std::make_shared<binpac::Grammar>("my-grammar", root);
    g->printTables(std::cout, true);

    auto error = g->check();

    if ( error.size() )
        std::cout << "Grammar error: " << error << std::endl;
}
