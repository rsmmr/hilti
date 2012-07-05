
#include "module.h"

using namespace hilti;

Module::module_map Module::_modules;

Module::Module(shared_ptr<ID> id, const string& path, const Location& l)
    : ast::Module<AstInfo>(id, path, l)
{
    shared_ptr<hilti::statement::Block> body(new hilti::statement::Block(nullptr, nullptr, l));
    setBody(body);

    // Implicitly always import the internal libhilti module.
    import("libhilti");

    // Implicitly always export Main::run().
    if ( util::strtolower(id->name()) == "main" )
        exportID("run", true);

    _modules.insert(make_pair(path, this));
}

shared_ptr<type::Context> Module::context() const
{
    auto expr = body()->scope()->lookup(std::make_shared<ID>("Context"), false);

    if ( ! expr )
        return nullptr;

    auto texpr = ast::as<expression::Type>(expr);
    assert(texpr);
    auto ctx = ast::as<type::Context>(texpr->typeValue());
    assert(ctx);

    return ctx;
}

shared_ptr<Module> Module::getExistingModule(const string& path)
{
    auto i = _modules.find(path);
    return (i != _modules.end()) ? i->second->sharedPtr<Module>() : shared_ptr<Module>();
}

