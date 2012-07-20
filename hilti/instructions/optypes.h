
#ifndef HILTI_INSTRUCTIONS_TYPE_H
#define HILTI_INSTRUCTIONS_TYPE_H

namespace optype {

inline shared_ptr<Type> optional(shared_ptr<Type> arg) {
    return std::make_shared<type::OptionalArgument>(arg);
}

static shared_ptr<Type> any(new type::Any());

static shared_ptr<Type> integer(new type::Integer());
static shared_ptr<Type> int8(new type::Integer(8));
static shared_ptr<Type> int16(new type::Integer(16));
static shared_ptr<Type> int32(new type::Integer(32));
static shared_ptr<Type> int64(new type::Integer(64));

static shared_ptr<Type> address(new type::Address());
static shared_ptr<Type> bitset(new type::Bitset());
static shared_ptr<Type> boolean(new type::Bool());
static shared_ptr<Type> bytes(new type::Bytes());
static shared_ptr<Type> caddr(new type::CAddr());
static shared_ptr<Type> callable(new type::Callable());
static shared_ptr<Type> channel(new type::Channel());
static shared_ptr<Type> classifier(new type::Classifier());
static shared_ptr<Type> context(new type::Context());
static shared_ptr<Type> double_(new type::Double());
static shared_ptr<Type> enum_(new type::Enum());
static shared_ptr<Type> exception(new type::Exception());
static shared_ptr<Type> file(new type::File());
static shared_ptr<Type> function(new type::Function());
static shared_ptr<Type> hook(new type::Hook());
static shared_ptr<Type> interval(new type::Interval());
static shared_ptr<Type> iosource(new type::IOSource());
static shared_ptr<Type> label(new type::Label());
static shared_ptr<Type> list(new type::List());
static shared_ptr<Type> map(new type::Map());
static shared_ptr<Type> matchTokenState(new type::MatchTokenState());
static shared_ptr<Type> network(new type::Network());
static shared_ptr<Type> overlay(new type::Overlay());
static shared_ptr<Type> port(new type::Port());
static shared_ptr<Type> regexp(new type::RegExp());
static shared_ptr<Type> scope(new type::Scope());
static shared_ptr<Type> set(new type::Set());
static shared_ptr<Type> string(new type::String());
static shared_ptr<Type> struct_(new type::Struct());
static shared_ptr<Type> time(new type::Time());
static shared_ptr<Type> timer(new type::Timer());
static shared_ptr<Type> timerMgr(new type::TimerMgr());
static shared_ptr<Type> tuple(new type::Tuple());
static shared_ptr<Type> vector(new type::Vector());

static shared_ptr<Type> iterBytes(new type::iterator::Bytes());
static shared_ptr<Type> iterIOSource(new type::iterator::IOSource());
static shared_ptr<Type> iterList(new type::iterator::List(std::make_shared<type::List>()));
static shared_ptr<Type> iterMap(new type::iterator::Map(std::make_shared<type::Map>()));
static shared_ptr<Type> iterSet(new type::iterator::Set(std::make_shared<type::Set>()));
static shared_ptr<Type> iterVector(new type::iterator::Vector(std::make_shared<type::Vector>()));

static shared_ptr<Type> refAny(new type::Reference());
static shared_ptr<Type> refBytes(new type::Reference(bytes));
static shared_ptr<Type> refCallable(new type::Reference(callable));
static shared_ptr<Type> refChannel(new type::Reference(channel));
static shared_ptr<Type> refClassifier(new type::Reference(classifier));
static shared_ptr<Type> refContext(new type::Reference(context));
static shared_ptr<Type> refException(new type::Reference(exception));
static shared_ptr<Type> refFile(new type::Reference(file));
static shared_ptr<Type> refIOSource(new type::Reference(iosource));
static shared_ptr<Type> refList(new type::Reference(list));
static shared_ptr<Type> refMap(new type::Reference(map));
static shared_ptr<Type> refMatchTokenState(new type::Reference(matchTokenState));
static shared_ptr<Type> refOverlay(new type::Reference(overlay));
static shared_ptr<Type> refRegExp(new type::Reference(regexp));
static shared_ptr<Type> refSet(new type::Reference(set));
static shared_ptr<Type> refStruct(new type::Reference(struct_));
static shared_ptr<Type> refTimer(new type::Reference(timer));
static shared_ptr<Type> refTimerMgr(new type::Reference(timerMgr));
static shared_ptr<Type> refVector(new type::Reference(vector));

static shared_ptr<Type> typeBytes(new type::TypeType(bytes));
static shared_ptr<Type> typeCallable(new type::TypeType(callable));
static shared_ptr<Type> typeChannel(new type::TypeType(channel));
static shared_ptr<Type> typeClassifier(new type::TypeType(classifier));
static shared_ptr<Type> typeException(new type::TypeType(exception));
static shared_ptr<Type> typeFile(new type::TypeType(file));
static shared_ptr<Type> typeIOSource(new type::TypeType(iosource));
static shared_ptr<Type> typeList(new type::TypeType(list));
static shared_ptr<Type> typeMap(new type::TypeType(map));
static shared_ptr<Type> typeMatchTokenState(new type::TypeType(matchTokenState));
static shared_ptr<Type> typeOverlay(new type::TypeType(overlay));
static shared_ptr<Type> typeRegExp(new type::TypeType(regexp));
static shared_ptr<Type> typeSet(new type::TypeType(set));
static shared_ptr<Type> typeStruct(new type::TypeType(struct_));
static shared_ptr<Type> typeTimer(new type::TypeType(timer));
static shared_ptr<Type> typeTimerMgr(new type::TypeType(timerMgr));
static shared_ptr<Type> typeVector(new type::TypeType(vector));


}

#endif
