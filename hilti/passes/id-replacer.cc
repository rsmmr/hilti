
#include "../expression.h"
#include "../id.h"
#include "../module.h"

#include "id-replacer.h"

using namespace hilti;
using namespace passes;

bool IDReplacer::run(shared_ptr<hilti::Node> node, shared_ptr<ID> old_id, shared_ptr<ID> new_id)
{
    if ( old_id->pathAsString() == new_id->pathAsString() )
        return true;

    _old_id = old_id;
    _new_id = new_id;

    return run(node);
}

bool IDReplacer::run(shared_ptr<Node> ast)
{
    return processAllPreOrder(ast);
}

void IDReplacer::visit(expression::ID* id)
{
    if ( id->id()->pathAsString() == _old_id->pathAsString() ) {
        auto nid = std::make_shared<ID>(_new_id->pathAsString(), _old_id->location());
        id->id()->replace(nid);
    }
}

void IDReplacer::visit(expression::Constant* c)
{
    auto label = ast::rtti::tryCast<constant::Label>(c->constant());

    if ( ! label )
        return;

    if ( label->value() == _old_id->pathAsString() ) {
        auto nlabel =
            std::make_shared<constant::Label>(_new_id->pathAsString(), _old_id->location());
        c->constant()->replace(nlabel);
    }
}
