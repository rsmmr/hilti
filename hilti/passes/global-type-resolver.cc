
#include "hilti/hilti-intern.h"

using namespace hilti::passes;

GlobalTypeResolver::~GlobalTypeResolver()
{
}

bool GlobalTypeResolver::run(shared_ptr<Node> module)
{
    _module = ast::rtti::checkedCast<Module>(module);
    _pass = 1;
    processAllPreOrder(module);
    _pass = 2;
    processAllPreOrder(module);

    return true;
}

void GlobalTypeResolver::visit(declaration::Type* t)
{
    if ( _pass == 2 )
        return;

    auto idx = t->id()->pathAsString(_module->id());
    _types.insert(std::make_pair(idx, t->type()));
}

void GlobalTypeResolver::visit(type::Unknown* t)
{
    if ( _pass == 1 )
        return;

    auto id = t->id();

    if ( ! id )
        return;

    auto idx = id->pathAsString(_module->id());
    auto i = _types.find(idx);

    if ( i == _types.end() )
        return;

    auto tv = i->second;

    t->replace(tv);

    if ( tv->id() ) {
        if ( ! tv->id()->isScoped() )
            tv->setID(id);
    }
    else
        tv->setID(id);
}
