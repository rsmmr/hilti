
#include "instructions/define-instruction.h"

#include "flow.h"
#include "module.h"
#include "builder/nodes.h"

iBeginCC(flow)
    iValidateCC(ReturnResult) {
        auto decl = validator()->current<Declaration>();

        if ( decl && ast::tryCast<declaration::Hook>(decl) ) {
            error(nullptr, "return.result must not be used inside a hook; use hook.stop instead");
            return;
        }
    }

    iDocCC(ReturnResult, "")
iEndCC

iBeginCC(flow)
    iValidateCC(ReturnVoid) {
    }

    iDocCC(ReturnVoid, "")
iEndCC

iBeginCC(flow)
    iValidateCC(BlockEnd) {
    }

    iDocCC(BlockEnd, "Internal instruction marking the end of a block that doesn't have any other terminator.")
iEndCC

iBeginCC(flow)
    iValidateCC(CallVoid) {
        auto ftype = as<type::Function>(op1->type());
        auto rtype = ftype->result()->type();
        shared_ptr<Type> none = nullptr;
        checkCallParameters(ftype, op2);
        checkCallResult(rtype, none);
    }

    iDocCC(CallVoid, "")
iEndCC

iBeginCC(flow)
    iValidateCC(CallResult) {
        auto ftype = as<type::Function>(op1->type());
        auto rtype = ftype->result()->type();
        checkCallParameters(ftype, op2);
        checkCallResult(rtype, target->type());
    }

    iDocCC(CallResult, "")
iEndCC

iBeginCC(flow)
    iValidateCC(CallCallableResult) {
        auto ctype = ast::as<type::Callable>(referencedType(op1));
        checkCallResult(ctype->argType(), target->type());
    }

    iDocCC(CallCallableResult, "")
iEndCC

iBeginCC(flow)
    iValidateCC(CallCallableVoid) {
        auto ctype = ast::as<type::Callable>(referencedType(op1));
        shared_ptr<Type> none = nullptr;
        checkCallResult(ctype->argType(), none);
    }

    iDocCC(CallCallableVoid, "")
iEndCC

iBeginCC(flow)
    iValidateCC(Yield) {
    }

    iDocCC(Yield, R"(
        Yields processing back to the current scheduler, to be resumed later.
        If running in a virtual thread other than zero, this instruction yields
        to other virtual threads running within the same physical thread. If
        running in virtual thread zero (or in non-threading mode), returns
        execution back to the calling C function (see interfacing with C).
    )")
iEndCC

iBeginCC(flow)
    iValidateCC(YieldUntil) {
        shared_ptr<Type> ty = op1->type();
        auto rtype = ast::as<type::Reference>(ty);

        if ( rtype )
            ty = rtype->argType();

        if ( ! type::hasTrait<type::trait::Blockable>(ty) )
            error(op1, "operand type does not support yield.until");
    }

    iDocCC(YieldUntil, R"(
        TODO.
    )")
iEndCC

iBeginCC(flow)
    iValidateCC(IfElse) {
    }

    iDocCC(IfElse, R"(
        Transfers control label *op2* if *op1* is true, and to *op3*
        otherwise.
    )")

iEndCC

iBeginCC(flow)
    iValidateCC(Jump) {
    }

    iDocCC(Jump, R"(
        Jumps unconditionally to label *op2*.
    )")

iEndCC

iBeginCC(flow)
    iValidateCC(Switch) {
        auto ty_op1 = as<type::ValueType>(op1->type());
        auto ty_op2 = as<type::Label>(op2->type());
        auto ty_op3 = as<type::Tuple>(op3->type());

        if ( ! ty_op1 ) {
            error(op1, "switch operand must be a value type");
            return;
        }

        isConstant(op3);

        auto a1 = ast::as<expression::Constant>(op3);
        auto a2 = ast::as<constant::Tuple>(a1->constant());

        for ( auto i : a2->value() ) {
            auto t = ast::as<type::Tuple>(i->type());

            if ( ! t ) {
                error(i, "not a tuple");
                return;
            }

            if ( t->typeList().size() != 2 ) {
                error(i, "switch clause must be a tuple (value, label)");
                return;
            }

            isConstant(i);

            auto c1 = ast::as<expression::Constant>(i);
            auto c2 = ast::as<constant::Tuple>(c1->constant());

            auto list = c2->value();
            auto j = list.begin();
            auto t1 = *j++;
            auto t2 = *j++;

            canCoerceTo(t1, ty_op1);
            equalTypes(t2->type(), builder::label::type());
        }
    }

    iDocCC(Switch, R"(
        Branches to one of several alternatives. *op1* determines which
        alternative to take.  *op3* is a tuple of giving all alternatives as
        2-tuples *(value, destination)*. *value* must be of the same type as
        *op1*, and *destination* is a block label. If *value* equals *op1*,
        control is transfered to the corresponding block. If multiple
        alternatives match *op1*, one of them is taken but it's undefined
        which one. If no alternative matches, control is transfered to block
        *op2*.
    )")

iEndCC

