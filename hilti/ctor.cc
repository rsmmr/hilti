
#include "id.h"
#include "ctor.h"

using namespace hilti;

shared_ptr<Type> ctor::Bytes::type() const
{
    return shared_ptr<type::Bytes>(new type::Bytes(location()));
}
