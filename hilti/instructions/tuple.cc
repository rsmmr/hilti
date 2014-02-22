
#include "define-instruction.h"

#include "tuple.h"
#include "../module.h"
#include "../type.h"
#include "../expression.h"

iBeginCC(tuple)
    iValidateCC(Equal) {
        equalTypes(op1, op2);
        // TODO: Check that we can compare the tuple element recursively.
    }

    iDocCC(Equal, R"(
        Returns True if the tuple in *op1* equals the tuple in *op2*.
    )")
iEndCC

iBeginCC(tuple)
    iValidateCC(Index) {
        auto ty_target = target->type();
        auto ttype = ast::as<type::Tuple>(op1->type());

        isConstant(op2);

        auto c = ast::as<expression::Constant>(op2)->constant();
        auto idx = ast::as<constant::Integer>(c)->value();

        if ( idx < 0 || (size_t) idx >= ttype->typeList().size() ) {
            error(op2, "tuple index out of range");
            return;
        }

        int i = 0;
        for ( auto t : ttype->typeList() ) {
            if ( i++ == idx )
                canCoerceTo(t, target);
        }
    }

    iDocCC(Index, R"(
       Returns the tuple's value with index *op2*. The index is zero-based. *op2* must be an integer *constant*.
    )")
iEndCC

iBeginCC(tuple)
    iValidateCC(Length) {
    }

    iDocCC(Length, R"(
       Returns the number of elements that the tuple *op1* has.
    )")
iEndCC

