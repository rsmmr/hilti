
#include "operator-resolver.h"

#include "operator.h"
#include "expression.h"

using namespace binpac;
using namespace binpac::passes;

static string _fmtOps(const expression_list& ops)
{
    string s = "(";

    for ( auto o : ops ) {
        if ( s.size() > 1 )
            s += ", ";

        s += o->type()->render();
    }

    return s + ")";
}

static string _fmtOpCandidates(const operator_list& candidates, const expression_list& ops)
{
    string s = "";

    s += "got operands: " + _fmtOps(ops) + "\n";
    s += "candidate operators:\n";

    for ( auto c : candidates )
        s += "    " + c->render() + "\n";

    return s;
}


OperatorResolver::OperatorResolver() : Pass<AstInfo>("OperatorResolver")
{
}

OperatorResolver::~OperatorResolver()
{
}

bool OperatorResolver::run(shared_ptr<ast::NodeBase> module)
{
    // We iterate until we don't see a change anymore. This takes into
    // account that resolving one operator may now allow us to do another one
    // depending on it.

    int last_unknowns = -1;

    do {
        last_unknowns = _unknowns.size();
        _unknowns.clear();

        if ( ! processAllPostOrder(module) )
            return false;

    } while ( _unknowns.size() && _unknowns.size() != last_unknowns );

    for ( auto o : _unknowns ) {
        auto all = OperatorRegistry::globalRegistry()->byKind(o->kind());
        error(o, "no matching operator found for types\n" + _fmtOpCandidates(all, o->operands()));
    }

    return _unknowns.size() == 0;
}

void OperatorResolver::visit(expression::UnresolvedOperator* o)
{
    auto ops = o->operands();
    auto matches = OperatorRegistry::globalRegistry()->getMatching(o->kind(), ops);

    switch ( matches.size() ) {
     case 0: {
         // Record for now, we may resolve it later.
         _unknowns.push_back(o);
         break;
     }

     case 1: {
         // Everthing is fine. Replace with the actual operator.
         auto nop = OperatorRegistry::globalRegistry()->resolveOperator(*matches.begin(), o->operands(), o->location());
         o->replace(nop);
         break;
     }

     default:
        error(o, "operator use is ambigious\n" + _fmtOpCandidates(matches, ops));
        break;
    }
}

void OperatorResolver::visit(Variable* i)
{
    if ( i->init() && ast::isA<type::Unknown>(i->type()) && ! ast::isA<type::Unknown>(i->init()->type()) )
        // We should have resolved the init expression by now.
        i->setType(i->init()->type());
}
