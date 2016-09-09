
#include "Pac2AST.h"

#include <binpac/binpac++.h>
#include <binpac/declaration.h>
#include <binpac/type.h>

using namespace bro::hilti;

void Pac2AST::process(shared_ptr<Pac2ModuleInfo> arg_minfo, shared_ptr<binpac::Module> module)
{
    minfo = arg_minfo;
    processAllPreOrder(module);
}

shared_ptr<Pac2AST::UnitInfo> Pac2AST::LookupUnit(const string& id)
{
    auto i = units.find(id);
    return i != units.end() ? i->second : nullptr;
}

void Pac2AST::visit(binpac::Module* u)
{
}

void Pac2AST::visit(binpac::declaration::Type* t)
{
    shared_ptr<binpac::type::Unit> unit = ast::rtti::tryCast<binpac::type::Unit>(t->type());

    if ( ! unit )
        return;

    string name = t->id()->name();
    string fq = current<binpac::Module>()->id()->name() + "::" + name;

    auto uinfo = std::make_shared<UnitInfo>();
    uinfo->name = fq;
    uinfo->exported = (t->linkage() == binpac::Declaration::EXPORTED);
    uinfo->unit_type = unit;
    uinfo->minfo = minfo;

    units.insert(std::make_pair(fq, uinfo));
}
