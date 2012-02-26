
#ifndef HILTI_INSTRUCTIONS_TYPE_H
#define HILTI_INSTRUCTIONS_TYPE_H

namespace optype {

static shared_ptr<Type> any(new type::Any());

static shared_ptr<Type> boolean(new type::Bool());
static shared_ptr<Type> string(new type::String());
static shared_ptr<Type> function(new type::Function());
static shared_ptr<Type> label(new type::Label());
static shared_ptr<Type> tuple(new type::Tuple());

static shared_ptr<Type> refBytes(new type::Reference(shared_ptr<Type>(new type::Bytes())));

static shared_ptr<Type> iterBytes(new type::iterator::Bytes());

static shared_ptr<Type> integer(new type::Integer());
static shared_ptr<Type> int8(new type::Integer(8));
static shared_ptr<Type> int16(new type::Integer(16));
static shared_ptr<Type> int32(new type::Integer(32));
static shared_ptr<Type> int64(new type::Integer(64));

static shared_ptr<Type> typeBytes(new type::Type(shared_ptr<Type>(new type::Bytes())));

}

#endif
