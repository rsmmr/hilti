
#include "declaration.h"
#include "statement.h"
#include "variable.h"
#include "function.h"
#include "instruction.h"
#include "expression.h"
#include "passes/printer.h"

string Expression::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());

    string r = s.str();

    if ( scope().size() )
        r = r + " " + util::fmt("[scope: %s]", scope().c_str());

    return r;
}
