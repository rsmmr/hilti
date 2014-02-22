
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

    bool replace_with_global = false;

    shared_ptr<ast::NodeBase> parent = nullptr;
    shared_ptr<ast::NodeBase> child_of_parent = nullptr;
    shared_ptr<ast::NodeBase> child_of_child_of_parent = nullptr;
    shared_ptr<ast::NodeBase> child_of_child_of_child_of_parent = nullptr;
    shared_ptr<ast::NodeBase> direct_parent = nullptr;

    for ( Pass::node_list::const_reverse_iterator i = nodes.rbegin();
          i != nodes.rend(); i++ ) {

        if ( ! (ast::isA<Expression>(*i) || ast::isA<Constant>(*i)) && (*i).get() != c ) {
            parent = *i;
            break;
        }

        child_of_child_of_child_of_parent = child_of_child_of_parent;
        child_of_child_of_parent = child_of_parent;
        child_of_parent = *i;

        if ( ! direct_parent && (*i).get() != c )
            direct_parent = *i;
    }

    if ( ! (parent && child_of_parent) )
        // Nothing found, probably won't happen annyways.
        return;

    if ( ! direct_parent )
        direct_parent = parent;

    // If it's an instruction, determine if the child we came through here
    // (i.e., the operand) is a constant.

    if ( auto stmt = ast::tryCast<statement::instruction::Resolved>(parent) ) {
        auto ins = stmt->instruction();

        if ( stmt->op1() == child_of_parent )
            replace_with_global = ins->typeOperand(1).second;

        if ( stmt->op2() == child_of_parent )
            replace_with_global = ins->typeOperand(2).second;

        if ( stmt->op3() == child_of_parent )
            replace_with_global = ins->typeOperand(3).second;

        // For function arguments, check against the signature.

        shared_ptr<hilti::type::Function> ftype = nullptr;
        shared_ptr<hilti::Expression> tuple_op = nullptr;

        if ( ast::isA<statement::instruction::flow::CallResult>(stmt) ||
             ast::isA<statement::instruction::flow::CallVoid>(stmt) ||
             ast::isA<statement::instruction::thread::Schedule>(stmt) ) {
            ftype = ast::checkedCast<type::Function>(stmt->op1()->type());
            tuple_op = stmt->op2();
        }

        if ( ast::isA<statement::instruction::flow::CallCallableResult>(stmt) ||
             ast::isA<statement::instruction::flow::CallCallableVoid>(stmt) ) {
            auto rt = ast::checkedCast<type::Reference>(stmt->op1()->type());
            ftype = ast::checkedCast<type::Callable>(rt->argType());
            tuple_op = stmt->op2();
        }

        if ( ast::isA<statement::instruction::hook::Run>(stmt) ) {
            ftype = ast::checkedCast<type::Hook>(stmt->op1()->type());
            tuple_op = stmt->op2();
        }

        if ( ftype && tuple_op && tuple_op.get() == child_of_parent.get() ) {
            auto args_expr = ast::checkedCast<expression::Constant>(tuple_op);
            auto args_tuple = ast::checkedCast<constant::Tuple>(args_expr->constant());

            auto fparams = ftype->parameters();
            auto p = fparams.begin();

            for ( auto a : args_tuple->value() ) {
                if ( a.get() == child_of_child_of_child_of_parent.get() && (*p)->constant() )
                    replace_with_global = true;

                ++p;
            }
        }
    }

    // Now, if we can replace it with a global, do so.

    if ( ! replace_with_global )
        return;

    auto name = ::util::fmt("__opt_ctor_%d", ++_id_counter);
    auto id = std::make_shared<ID>(name, c->location());

    auto scope = _module->body()->scope();

    // Note, we don't cache once-created object here for reuse because that
    // could change the semantics of "equal ctor1 ctor2".

    auto init = std::make_shared<expression::Ctor>(c->ctor());
    auto var = std::make_shared<variable::Global>(id, c->type(), init, c->location());
    auto decl = std::make_shared<declaration::Variable>(id, var, c->location());
    auto nexpr = std::make_shared<expression::Variable>(var, c->location());

    _module->body()->addDeclaration(decl);
    scope->insert(id, nexpr);

    c->replace(nexpr, direct_parent);
}


