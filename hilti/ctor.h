
#ifndef HILTI_CTOR_H
#define HILTI_CTOR_H

#include <stdint.h>

#include "common.h"
#include "scope.h"
#include "type.h"
#include "visitor-interface.h"

namespace hilti {

/// Base class for ctor nodes. A ctor instantiates a HeapType.
class Ctor : public ast::Ctor<AstInfo>
{
public:
   /// Constructor.
   ///
   /// l: An associated location.
   Ctor(const Location& l=Location::None)
       : ast::Ctor<AstInfo>(l) {}

   ACCEPT_VISITOR_ROOT();
};

namespace ctor {

/// AST node for a bytes constructor.
class Bytes : public Ctor
{
public:
   /// Constructor.
   ///
   /// b: The value to initialize the bytes object with.
   ///
   /// l: An associated location.
   Bytes(const string& b, const Location& l=Location::None) : Ctor(l) {
       _value = b;
   }

   /// Returns the initialization value.
   const string& value() const { return _value; }

   /// Returns the type of the constructed object.
   shared_ptr<Type> type() const override;

   ACCEPT_VISITOR(Ctor);

private:
   string _value;
};


}

}

#endif
