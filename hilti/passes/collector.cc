
#include "../declaration.h"
#include "../variable.h"
#include "../statement.h"
#include "../module.h"

#include "collector.h"

using namespace hilti;
using namespace passes;

void Collector::visit(declaration::Variable* v)
{
    if ( ! in<Function>() ) {
        // A global.
        auto var = v->variable();
        _globals.push_back(var);
        debug(1, string("global: ") + var->id()->name());
    }
}

static bool compare_vars(shared_ptr<Variable> v1, shared_ptr<Variable> v2)
{
    return v1->id()->pathAsString() <= v2->id()->pathAsString();
}

const Collector::var_list& Collector::globals()
{
    assert(has_run);
    _globals.sort(compare_vars);
    return _globals;
}
