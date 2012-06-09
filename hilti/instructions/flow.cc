
#include "instructions/define-instruction.h"

#include "flow.h"
#include "module.h"
#include "builder/nodes.h"

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

