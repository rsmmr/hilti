
#include "id.h"
#include "ctor.h"

using namespace hilti;

shared_ptr<Type> ctor::Bytes::type() const
{
    auto b = shared_ptr<type::Bytes>(new type::Bytes(location()));
    return shared_ptr<type::Reference>(new type::Reference(b, location()));
}
