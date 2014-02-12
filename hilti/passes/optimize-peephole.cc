
#include "../statement.h"
#include "../module.h"
#include "../builder/nodes.h"

#include "optimize-peephole.h"
#include "printer.h"
#include "hilti/autogen/instructions.h"

using namespace hilti;
using namespace passes;

OptimizePeepHole::OptimizePeepHole() : Pass<>("hilti::OptimizePeepHole", true)
{
}

bool OptimizePeepHole::run(shared_ptr<hilti::Node> module)
{
    return processAllPreOrder(module);
}
