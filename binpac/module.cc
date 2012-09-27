
#include "module.h"
#include "statement.h"
#include "type.h"

using namespace binpac;

Module::Module(CompilerContext* ctx, shared_ptr<ID> id, const string& path, const Location& l)
    : ast::Module<AstInfo>(id, path, l)
{
    _context = ctx;
    auto body = std::make_shared<binpac::statement::Block>(nullptr, l);
    setBody(body);

    // Implicitly always import the BinPAC module.
    import("binpac");
}

CompilerContext* Module::context() const
{
    return _context;
}
