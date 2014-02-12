
#include "../statement.h"
#include "../module.h"
#include "../builder/nodes.h"

#include "optimize-ctors.h"
#include "printer.h"
#include "hilti/autogen/instructions.h"

using namespace hilti;
using namespace passes;

int OptimizeCtors::_id_counter = 0;

OptimizeCtors::OptimizeCtors() : Pass<>("hilti::OptimizeCtors", true)
{
}

bool OptimizeCtors::run(shared_ptr<hilti::Node> module)
{
    _module = ast::checkedCast<Module>(module);
    return processAllPreOrder(module);
}

void OptimizeCtors::visit(expression::Ctor* c)
{
    // Find the closest non-expression node we have traversed to get here. We
    // also keep the child right after that parent.
    auto nodes = currentNodes();

    std::string x = ::util::fmt("*** %s",c->render());
    bool replace_with_global = false;

    shared_ptr<ast::NodeBase> parent = nullptr;
    shared_ptr<ast::NodeBase> child_of_parent = nullptr;
    shared_ptr<ast::NodeBase> direct_parent = nullptr;

    for ( Pass::node_list::const_reverse_iterator i = nodes.rbegin();
          i != nodes.rend(); i++ ) {

        if ( ! (ast::isA<Expression>(*i) || ast::isA<Constant>(*i)) && (*i).get() != c ) {
            parent = *i;
            break;
        }

        child_of_parent = *i;

        if ( ! direct_parent && (*i).get() != c )
            direct_parent = *i;
    }

    //fprintf(stderr, "?1 %s -> %d (%p/%p/%p)\n", x.c_str(), (int) replace_with_global,
//            parent.get(), direct_parent.get(), child_of_parent.get());

    if ( ! (parent && child_of_parent) )
        // Nothing found, probably won't happen annyways.
        return;

    if ( ! direct_parent )
        direct_parent = parent;

    // If it's an instruction, determine if the child we came through here
    // (i.e., the operand) is a constant.

    if ( auto ins = ast::tryCast<statement::instruction::Resolved>(parent) ) {
        if ( ins->op1() == child_of_parent )
            replace_with_global = ins->instruction()->typeOperand(1).second;

        if ( ins->op2() == child_of_parent )
            replace_with_global = ins->instruction()->typeOperand(2).second;

        if ( ins->op3() == child_of_parent )
            replace_with_global = ins->instruction()->typeOperand(3).second;

        x = ::util::fmt(" %s | %d %d %d (%p/%p/%p)\n", string(*ins).c_str(),
           (int)ins->instruction()->typeOperand(1).second,
           (int)ins->instruction()->typeOperand(2).second,
           (int)ins->instruction()->typeOperand(3).second,
           ins->op1().get(), ins->op2().get(), ins->op3().get()
           );

    }

    // TODO: Check for constant parameters to function calls.

    // Now, if we can replace it with a global, do so.

//fprintf(stderr, "?2 %s -> %d (%p/%p/%p)\n", x.c_str(), (int) replace_with_global,
//            parent.get(), direct_parent.get(), child_of_parent.get());

    if ( ! replace_with_global )
        return;

    auto name = ::util::fmt("__opt_ctor_%d", ++_id_counter);
    auto id = std::make_shared<ID>(name, c->location());

    auto scope = _module->body()->scope();

#if 0
    // Note, we don't cache once-created object here for reuse because that
    // would change the semantics of "equal ctor1 ctor2". Because equal does
    // ptr comparision, not a deep comparision, we get false for two ctors
    // that create otherwise identical objects.
    //
    // If we ever changed that to deep copies, we could cache here. Then
    // we need to change the name from a counter to using maybe the pointer
    // or a string representation.

    auto nexpr = scope->lookupUnique(id);

    if ( ! nexpr )
#endif

    auto init = std::make_shared<expression::Ctor>(c->ctor());
    auto var = std::make_shared<variable::Global>(id, c->type(), init, c->location());
    auto decl = std::make_shared<declaration::Variable>(id, var, c->location());
    auto nexpr = std::make_shared<expression::Variable>(var, c->location());

    _module->body()->addDeclaration(decl);
    scope->insert(id, nexpr);

#if 0
    }
#endif

// fprintf(stderr, "%s -> %s | %s\n", c->render().c_str(), nexpr->render().c_str(), x.c_str());

    c->replace(nexpr, direct_parent);
}


