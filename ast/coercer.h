
#ifndef AST_COERCER_H
#define AST_COERCER_H

#include "type.h"
#include "visitor.h"

namespace ast {

// Coercer base class. Instances of this visitor check whether one type can
// be coerced into another.
template<typename AstInfo>
class Coercer : public Visitor<AstInfo, bool, shared_ptr<typename AstInfo::type>>
{
public:
    typedef typename AstInfo::type Type;
    typedef typename AstInfo::any_type AnyType;
    typedef typename AstInfo::optional_type OptionalType;

    Coercer() { this->setDefaultResult(false); }

    virtual ~Coercer() {};

    // Checks whether arbitrary expressions of one type can be coerced into
    // that of another. The method calls the appropiate visit method for the
    // source type and returns the result it indicates. If src and dest are
    // equal (as determined by the Type comparision operator), the method
    // returns always true. If one of the two types has been marked as
    // matching any other (via Type::setMatchesAny), the result is also always
    // true.
    //
    // src: The source type.
    //
    // dst: The destination type.
    //
    // Returns: True of #src coerces into #dst.
    bool canCoerceTo(shared_ptr<Type> src, shared_ptr<Type> dst);
};

template<typename AstInfo>
inline bool Coercer<AstInfo>::canCoerceTo(shared_ptr<Type> src, shared_ptr<Type> dst)
{
    // Sometimes comparision isn't fully symmetrics, like with optional arguments.
    if ( src->equal(dst) || dst->equal(src) )
        return true;

    if ( isA<AnyType>(src) )
        return true;

    auto t = tryCast<OptionalType>(dst);

    if ( t )
        return canCoerceTo(src, t->argType());

    bool result;
    bool success = this->processOne(src, &result, dst);
    assert(success);
    return result;
}

}

#endif
