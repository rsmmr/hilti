
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

        s += o->render();
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
    return processAllPostOrder(module);
}

void OperatorResolver::visit(expression::UnresolvedOperator* o)
{
    auto ops = o->operands();
    auto matches = OperatorRegistry::globalRegistry()->getMatching(o->kind(), ops);

    switch ( matches.size() ) {
     case 0: {
         auto all = OperatorRegistry::globalRegistry()->byKind(o->kind());
         error(o, "no matching operator found for types\n" + _fmtOpCandidates(all, ops));
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
