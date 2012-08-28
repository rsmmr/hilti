
#include "id.h"

using namespace binpac;

ID::ID(string path, const Location& l) : ast::ID<AstInfo>(path, l)
{
}

ID::ID(component_list path, const Location& l) : ast::ID<AstInfo>(path, l)
{
}
