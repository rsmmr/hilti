
#include "scope.h"

using namespace spicy;

Scope::Scope(shared_ptr<Scope> parent) : ast::Scope<AstInfo>(parent)
{
}
