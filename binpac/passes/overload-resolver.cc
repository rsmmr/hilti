
#include "overload-resolver.h"
#include "id.h"
#include "type.h"
#include "function.h"
#include "expression.h"
#include "operator.h"
#include "statement.h"
#include "scope.h"
#include "autogen/operators/function.h"

using namespace binpac;
using namespace binpac::passes;

OverloadResolver::OverloadResolver() : Pass<AstInfo>("OverloadResolver")
{
}


OverloadResolver::~OverloadResolver()
{
}

bool OverloadResolver::run(shared_ptr<ast::NodeBase> module)
{
    return processAllPreOrder(module);
}

void OverloadResolver::visit(expression::UnresolvedOperator* o)
{
    // Resolve overlaoded function calls. We resolve this operator here
    // "manually" bevore the actual operator resolver kicks in in a different
    // pass.

    if ( o->kind() != operator_::Call )
        return;

    auto operands = o->operands();

    assert(operands.size() == 2);

    auto i = operands.begin();
    auto id = ast::tryCast<expression::ID>(*i++);
    auto args = ast::checkedCast<expression::List>(*i)->expressions();

    if ( ! id )
        return;

    // Find the nearest to scope.
    shared_ptr<Scope> scope = nullptr;

    auto nodes = currentNodes();

    for ( auto i = nodes.rbegin(); i != nodes.rend(); i++ ) {
        auto n = *i;

        auto unit = ast::tryCast<type::Unit>(n);
        auto item = ast::tryCast<type::unit::Item>(n);
        auto block = ast::tryCast<statement::Block>(n);

        if ( block ) {
            scope = block->scope();
            break;
        }

        if ( item ) {
            scope = item->scope();
            break;
        }

        if ( unit ) {
            scope = unit->scope();
            break;
        }
    }

    if ( ! scope ) {
        error(o, "ID expression outside of any scope");
        return;
    }

    auto vals = scope->lookup(id->id());

    assert(vals.size());

    std::list<shared_ptr<expression::Function>> candidates;

    // Resolve overloaded function calls.
    for ( auto v : vals ) {
        auto func = ast::tryCast<expression::Function>(v);

        if ( ! func )
            // Something else than a function, the validator will catch that.
            // Just do nothing here.
            return;

        auto proto = func->function()->type()->parameters();

        auto a = args.begin();
        auto p = proto.begin();

        while ( a != args.end() ) {
            if ( p == proto.end() )
                // More args given than accepted.
                goto no_match;

            if ( ! (*a)->canCoerceTo((*p)->type()) )
                goto no_match; // Type does not match.

            p++;
            a++;
        }

        while ( p != proto.end() ) {
            if ( ! (*p)->default_() )
                // Not enough args given.
                goto no_match;

            p++;
        }

        // Found a match.
        candidates.push_back(func);

no_match:
        continue;
    }

    if ( candidates.size() == 0 ){
        error(o, "No match for overloaded function call");
        return;
    }

    if ( candidates.size() > 1 ) {
        error(o, "Ambigious call of overloaded function");
        return;
    }

    // Resolve it.
    auto func = candidates.front();

    auto call = std::make_shared<operator_::function::Call>();
    expression_list noperands = { func, *i };

    auto nop = std::make_shared<expression::operator_::function::Call>(call, noperands, o->location());

    o->replace(nop);
}
