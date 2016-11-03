
#include "id.h"

using namespace spicy;

ID::ID(string path, const Location& l) : ast::ID<AstInfo>(path, l)
{
}

ID::ID(component_list path, const Location& l) : ast::ID<AstInfo>(path, l)
{
}

ID::~ID()
{
}
