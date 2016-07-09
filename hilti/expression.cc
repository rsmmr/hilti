
#include "expression.h"
#include "declaration.h"
#include "function.h"
#include "instruction.h"
#include "passes/printer.h"
#include "statement.h"
#include "variable.h"

bool Expression::hoisted()
{
    return false;
}

string Expression::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());

    string r = s.str();

    if ( scope().size() )
        r = r + " " + util::fmt("[scope: %s]", scope().c_str());

    return r;
}

std::list<shared_ptr<hilti::Expression>> expression::List::flatten()
{
    std::list<shared_ptr<hilti::Expression>> exprs;

    for ( auto e : expressions() )
        exprs.push_back(e);

    return exprs;
}

std::list<shared_ptr<hilti::Expression>> expression::Constant::flatten()
{
    std::list<shared_ptr<hilti::Expression>> l = {this->sharedPtr<hilti::Expression>()};

    if ( constant() )
        l.merge(constant()->flatten());

    return l;
}

std::list<shared_ptr<hilti::Expression>> expression::Ctor::flatten()
{
    std::list<shared_ptr<hilti::Expression>> l = {this->sharedPtr<hilti::Expression>()};

    if ( ctor() )
        l.merge(ctor()->flatten());

    return l;
}

bool expression::Variable::hoisted()
{
    return variable()->attributes().has(attribute::HOIST);
}
