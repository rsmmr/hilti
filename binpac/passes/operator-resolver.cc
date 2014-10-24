
#include "operator-resolver.h"
#include "id-resolver.h"

#include "../operator.h"
#include "../expression.h"

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

static string _fmtOpCandidates(const OperatorRegistry::matching_result& candidates, const expression_list& ops)
{
    string s = "";

    s += "got operands: " + _fmtOps(ops) + "\n";
    s += "candidate operators:\n";

    for ( auto c : candidates )
        s += "    " + c.first->render() + "\n";

    return s;
}


OperatorResolver::OperatorResolver(shared_ptr<Module> module) : Pass<AstInfo>("binpac::OperatorResolver", true)
{
    _module = module;
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
        OperatorRegistry::matching_result matches;

        for ( auto a : all )
            matches.push_back(std::make_pair(a, expression_list()));

        error(o, "no matching operator found for types\n" + _fmtOpCandidates(matches, o->operands()));
    }

    return _unknowns.size() == 0;
}

void OperatorResolver::visit(expression::UnresolvedOperator* o)
{
    auto matches = OperatorRegistry::globalRegistry()->getMatching(o->kind(), o->operands());

    switch ( matches.size() ) {
     case 0: {
         // Record for now, we may resolve it later.
         _unknowns.push_back(o);
         break;
     }

     case 1: {
         // Everthing is fine. Replace with the actual operator.
         auto match = matches.front();
         auto nop = OperatorRegistry::globalRegistry()->resolveOperator(match.first, match.second, _module, o->location());
         o->replace(nop);
         nop->setUsesTryAttribute(o->usesTryAttribute());
         break;
     }

     default:
        error(o, "operator use is ambigious\n" + _fmtOpCandidates(matches, o->operands()));
        break;
    }
}

void OperatorResolver::visit(Variable* i)
{
    if ( i->init() && ast::isA<type::Unknown>(i->type()) && ! ast::isA<type::Unknown>(i->init()->type()) )
        // We should have resolved the init expression by now.
        i->setType(i->init()->type());
}

void OperatorResolver::visit(type::UnknownElementType* u)
{
    auto t = u->expression()->type();

    if ( ast::isA<type::Unknown>(t) )
        return;

    auto iterable = ast::type::tryTrait<type::trait::Iterable>(t);

    if ( ! iterable ) {
        error(u->expression(), "expression not of iterable type");
        return;
    }

    auto etype = iterable->elementType();

    if ( ast::isA<type::Unknown>(etype) )
        return;

    u->replace(etype);
}
