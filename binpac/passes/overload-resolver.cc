
#include "overload-resolver.h"
#include "id.h"
#include "type.h"
#include "function.h"

using namespace binpac;
using namespace binpac::passes;

OverloadResolver::OverloadResolver() : Pass<AstInfo>("OverloadResolver")
{
}


OverloadResolver::~OverloadResolver()
{
}

bool OverloadResolver::run(shared_ptr<ast::NodeBase> module)
{
    return processAllPreOrder(module);
}

void OverloadResolver::visit(expression::ID* i)
{
}
