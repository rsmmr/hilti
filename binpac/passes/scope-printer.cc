
#include "../expression.h"
#include "../statement.h"
#include "../scope.h"

#include "scope-printer.h"
#include "printer.h"

using namespace binpac;
using namespace binpac::passes;

ScopePrinter::ScopePrinter(std::ostream& out) : Pass<AstInfo>("binpac::ScopePrinter", false), _out(out)
{
}

ScopePrinter::~ScopePrinter()
{
}

bool ScopePrinter::run(shared_ptr<ast::NodeBase> module)
{
    return processAllPreOrder(module);
}

void ScopePrinter::visit(statement::Block* b)
{
    _out << "<<< Scope for " << b->render() << std::endl;
    b->scope()->dump(_out);
    _out << ">>>" << std::endl << std::endl;
}

void ScopePrinter::visit(type::unit::Item * i)
{
    _out << "<<< Scope for " << i->render() << std::endl;
    i->scope()->dump(_out);
    _out << ">>>" << std::endl << std::endl;
}
