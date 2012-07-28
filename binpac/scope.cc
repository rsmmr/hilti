
#include "scope.h"

using namespace binpac;

Scope::Scope(shared_ptr<Scope> parent) : ast::Scope<AstInfo>(parent)
{
}
