
#include "module.h"
#include "statement.h"
#include "type.h"

using namespace binpac;

Module::Module(shared_ptr<ID> id, const string& path, const Location& l)
    : ast::Module<AstInfo>(id, path, l)
{
    auto body = std::make_shared<binpac::statement::Block>(nullptr, l);
    setBody(body);
}
