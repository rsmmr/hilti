
#include "../declaration.h"
#include "../module.h"
#include "../statement.h"
#include "../variable.h"

#include "collector.h"

using namespace hilti;
using namespace passes;

void Collector::visit(declaration::Variable* v)
{
    auto var = v->variable();

    if ( ast::rtti::isA<variable::Global>(var) && v->linkage() != Declaration::IMPORTED ) {
        // A global.
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
