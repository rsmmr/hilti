//
// @TEST-EXEC: ${SCRIPTS}/build-with-spicy %INPUT -o a.out
// @TEST-EXEC: ./a.out >out
// @TEST-EXEC: btest-diff out

#include <memory>

#include <spicy.h>
#include <hilti/hilti-intern.h>

#include <constant.h>
#include <expression.h>
#include <grammar.h>

int main(int argc, char** argv)
{
    spicy::init();

    auto c1 = std::make_shared<spicy::ctor::Bytes>("l1-val");
    auto l1 = std::make_shared<spicy::production::Ctor>("l1", c1);

    auto c2 = std::make_shared<spicy::ctor::Bytes>("l2-val");
    auto l2 = std::make_shared<spicy::production::Ctor>("l2", c2);

    auto c3 = std::make_shared<spicy::ctor::Bytes>("l3-val");
    auto l3 = std::make_shared<spicy::production::Ctor>("l3", c3);

    std::list<shared_ptr<spicy::Production>> seq = {l1, l2, l3};
    auto r = std::make_shared<spicy::production::Sequence>("S", seq);
    auto g = std::make_shared<spicy::Grammar>("my-grammar", r);

    g->printTables(std::cout, true);

    auto error = g->check();

    if ( error.size() )
        std::cout << "Grammar error: " << error << std::endl;
}
