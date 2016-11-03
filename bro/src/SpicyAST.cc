
#include "SpicyAST.h"

#include <spicy/spicy.h>
#include <spicy/declaration.h>
#include <spicy/type.h>

using namespace bro::hilti;

void SpicyAST::process(shared_ptr<SpicyModuleInfo> arg_minfo, shared_ptr<spicy::Module> module)
{
    minfo = arg_minfo;
    processAllPreOrder(module);
}

shared_ptr<SpicyAST::UnitInfo> SpicyAST::LookupUnit(const string& id)
{
    auto i = units.find(id);
    return i != units.end() ? i->second : nullptr;
}

void SpicyAST::visit(spicy::Module* u)
{
}

void SpicyAST::visit(spicy::declaration::Type* t)
{
    shared_ptr<spicy::type::Unit> unit = ast::rtti::tryCast<spicy::type::Unit>(t->type());

    if ( ! unit )
        return;

    string name = t->id()->name();
    string fq = current<spicy::Module>()->id()->name() + "::" + name;

    auto uinfo = std::make_shared<UnitInfo>();
    uinfo->name = fq;
    uinfo->exported = (t->linkage() == spicy::Declaration::EXPORTED);
    uinfo->unit_type = unit;
    uinfo->minfo = minfo;

    units.insert(std::make_pair(fq, uinfo));
}
