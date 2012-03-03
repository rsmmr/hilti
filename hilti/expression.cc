
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
    return s.str();
}
