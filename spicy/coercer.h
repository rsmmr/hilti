
#ifndef SPICY_COERCER_H
#define SPICY_COERCER_H

#include "common.h"
#include "type.h"

namespace spicy {

/// Visitor implementating the check whether an expression of one type can be
/// coerced into that of another. This class should not be used directly. The
/// frontend is Expression::canCoerceTo.
class Coercer {
public:
    /// Checks whether arbitrary expressions of one type can be coerced into
    /// that of another. The method calls the appropiate visit method for the
    /// source type and returns the result it indicates. If src and dest are
    /// equal (as determined by the Type comparision operator), the method
    /// returns always true. If one of the two types has been marked as
    /// matching any other (via Type::setMatchesAny), the result is also always
    /// true.
    ///
    /// src: The source type.
    ///
    /// dst: The destination type.
    ///
    /// error: If given, an error messages will be saved here if coercion isn't possible.
    ///
    /// Returns: True of #src coerces into #dst.
    bool canCoerceTo(shared_ptr<Type> src, shared_ptr<Type> dst, string* error = nullptr);

    shared_ptr<Expression> coerceTo(shared_ptr<Expression> expr, shared_ptr<Type> dst);

private:
    // Main internal work horse function.
    bool _coerceTo(shared_ptr<Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst,
                   shared_ptr<Expression>* result, string* error);
};
}

#endif
