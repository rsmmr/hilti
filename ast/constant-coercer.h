
#ifndef AST_CONSTANT_COERCER_H
#define AST_CONSTANT_COERCER_H

#include "constant.h"
#include "type.h"
#include "visitor.h"

namespace ast {

/// Class for coercing constants of one type into constants of another.
template<typename AstInfo>
class ConstantCoercer : public Visitor<AstInfo, shared_ptr<typename AstInfo::constant>, shared_ptr<typename AstInfo::type>> {
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::constant Constant;

   ConstantCoercer() {};
   virtual ~ConstantCoercer() {};

   // Checks whether a Constant of one type can be coerced into a Constant of
   // another. The method calls the appropiate visit method for the source
   // type and returns the result it indicates. If the constant's type equals
   // the destination type (as determined by the Type comparision operator),
   // the method returns always true. If the destination type has been marked
   // as matching any other (via Type::setMatchesAny), the result is also
   // always true.
   //
   // src: The constant to check for coercion.
   //
   // dst: The destination type.
   //
   // Returns: True of #src coerces into type #dst.
   bool canCoerceTo(shared_ptr<Constant> constant, shared_ptr<Type> dst) {
       return static_cast<bool>(coerceTo(constant, dst));
   }

   // Coerce a Constant of one type into a Constant of another. The method
   // calls the appropiate visit method for the source type and returns the
   // result it returns. If the constant's type equals the destination type
   // (as determined by the Type comparision operator), the method always
   // returns the original value. If the destination type has been marked as
   // matching any other (via Type::setMatchesAny), it likewise just passes the constant through.
   //
   // src: The constant to coerce.
   //
   // dst: The destination type.
   //
   // Returns: The coerced constant of type dst, or null if the constant does
   // not coerce into the destination type.
   shared_ptr<Constant> coerceTo(shared_ptr<Constant> constant, shared_ptr<Type> dst);
};

template<typename AstInfo>
inline shared_ptr<typename AstInfo::constant> ConstantCoercer<AstInfo>::coerceTo(shared_ptr<Constant> constant, shared_ptr<Type> dst)
{
    if ( constant->type()->equal(dst) )
        return constant;

    if ( dst->matchesAny() )
        return constant;

    this->setDefaultResult(nullptr);
    shared_ptr<typename AstInfo::constant> result;
    bool success = this->processOne(constant, &result, dst);
    assert(success);
    return result;
}

}

#endif
