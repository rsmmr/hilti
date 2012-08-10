//
// @TEST-EXEC: ${SCRIPTS}/build-with-binpac %INPUT -o a.out
// @TEST-EXEC: ./a.out >out
// @TEST-EXEC: btest-diff out
//

#include <pgen/grammar.h>
#include <constant.h>
#include <expression.h>

int main(int argc, char** argv)
{
    auto c1 = std::make_shared<binpac::expression::Constant>(std::make_shared<binpac::constant::String>("l1-val"));
    auto l1 = std::make_shared<binpac::production::Literal>("l1", c1);

    auto c2 = std::make_shared<binpac::expression::Constant>(std::make_shared<binpac::constant::String>("l2-val"));
    auto l2 = std::make_shared<binpac::production::Literal>("l2", c2);

    auto c3 = std::make_shared<binpac::expression::Constant>(std::make_shared<binpac::constant::String>("l3-val"));
    auto l3 = std::make_shared<binpac::production::Literal>("l3", c3);

    std::list<shared_ptr<binpac::Production>> seq = { l1, l2, l3 };
    auto r = std::make_shared<binpac::production::Sequence>("S", seq);
    auto g = std::make_shared<binpac::Grammar>("my-grammar", r);

    g->printTables(std::cout, true);

    auto error = g->check();

    if ( error.size() )
        std::cout << "Grammar error: " << error << std::endl;
}
