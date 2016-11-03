//
// @TEST-EXEC: ${SCRIPTS}/build-with-spicy -I %DIR %INPUT -o a.out
// @TEST-EXEC: ./a.out >out
// @TEST-EXEC: btest-diff out
//

#include <spicy.h>
#include <hilti/hilti-intern.h>

#include <constant.h>
#include <expression.h>
#include <grammar.h>

#include "production-helpers.cc"

int main(int argc, char** argv)
{
    spicy::init();

    //  Anohther simple example grammar mimicing HTTP headers. Error
    //  expected.

    auto hdrkey = ::variable("HdrKey", std::make_shared<spicy::type::Bytes>());
    auto hdrval = ::variable("HdrVal", std::make_shared<spicy::type::Bytes>());
    auto colon = literal("colon", ":");
    auto ws = literal("ws", "[ \\t]*");
    auto nl = literal("nl", "[\\r\\n]");
    auto header = sequence("Header", {hdrkey, ws, colon, ws, hdrval, nl});
    auto list1 = sequence("List1", {header});
    auto list2 = lookAhead("List2", list1, epsilon());
    list1->add(list2);
    auto all = lookAhead("All", list2, nl);

    auto g = std::make_shared<spicy::Grammar>("my-grammar", all);
    g->printTables(std::cout, true);

    auto error = g->check();

    if ( error.size() )
        std::cout << "Grammar error: " << error << std::endl;
}

