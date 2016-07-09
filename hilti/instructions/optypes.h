
#ifndef HILTI_INSTRUCTIONS_TYPE_H
#define HILTI_INSTRUCTIONS_TYPE_H

namespace hilti {
namespace optype {

// Initializes the globals at startup.
void __init();

inline shared_ptr<Type> optional(shared_ptr<Type> arg)
{
    return std::make_shared<type::OptionalArgument>(arg);
}

extern shared_ptr<Type> any;

extern shared_ptr<Type> integer;
extern shared_ptr<Type> int8;
extern shared_ptr<Type> int16;
extern shared_ptr<Type> int32;
extern shared_ptr<Type> int64;

extern shared_ptr<Type> address;
extern shared_ptr<Type> bitset;
extern shared_ptr<Type> boolean;
extern shared_ptr<Type> bytes;
extern shared_ptr<Type> caddr;
extern shared_ptr<Type> callable;
extern shared_ptr<Type> channel;
extern shared_ptr<Type> classifier;
extern shared_ptr<Type> context;
extern shared_ptr<Type> double_;
extern shared_ptr<Type> enum_;
extern shared_ptr<Type> exception;
extern shared_ptr<Type> file;
extern shared_ptr<Type> function;
extern shared_ptr<Type> hook;
extern shared_ptr<Type> interval;
extern shared_ptr<Type> iosource;
extern shared_ptr<Type> label;
extern shared_ptr<Type> list;
extern shared_ptr<Type> map;
extern shared_ptr<Type> matchTokenState;
extern shared_ptr<Type> network;
extern shared_ptr<Type> overlay;
extern shared_ptr<Type> port;
extern shared_ptr<Type> regexp;
extern shared_ptr<Type> scope;
extern shared_ptr<Type> set;
extern shared_ptr<Type> string;
extern shared_ptr<Type> struct_;
extern shared_ptr<Type> time;
extern shared_ptr<Type> timer;
extern shared_ptr<Type> timerMgr;
extern shared_ptr<Type> tuple;
extern shared_ptr<Type> union_;
extern shared_ptr<Type> vector;

extern shared_ptr<Type> iterBytes;
extern shared_ptr<Type> iterIOSource;
extern shared_ptr<Type> iterList;
extern shared_ptr<Type> iterMap;
extern shared_ptr<Type> iterSet;
extern shared_ptr<Type> iterVector;

extern shared_ptr<Type> refAny;
extern shared_ptr<Type> refBytes;
extern shared_ptr<Type> refCallable;
extern shared_ptr<Type> refChannel;
extern shared_ptr<Type> refClassifier;
extern shared_ptr<Type> refContext;
extern shared_ptr<Type> refException;
extern shared_ptr<Type> refFile;
extern shared_ptr<Type> refIOSource;
extern shared_ptr<Type> refList;
extern shared_ptr<Type> refMap;
extern shared_ptr<Type> refMatchTokenState;
extern shared_ptr<Type> refOverlay;
extern shared_ptr<Type> refRegExp;
extern shared_ptr<Type> refSet;
extern shared_ptr<Type> refStruct;
extern shared_ptr<Type> refTimer;
extern shared_ptr<Type> refTimerMgr;
extern shared_ptr<Type> refVector;

extern shared_ptr<Type> typeAny;
extern shared_ptr<Type> typeBytes;
extern shared_ptr<Type> typeCallable;
extern shared_ptr<Type> typeChannel;
extern shared_ptr<Type> typeClassifier;
extern shared_ptr<Type> typeException;
extern shared_ptr<Type> typeFile;
extern shared_ptr<Type> typeIOSource;
extern shared_ptr<Type> typeList;
extern shared_ptr<Type> typeMap;
extern shared_ptr<Type> typeMatchTokenState;
extern shared_ptr<Type> typeOverlay;
extern shared_ptr<Type> typeRegExp;
extern shared_ptr<Type> typeSet;
extern shared_ptr<Type> typeStruct;
extern shared_ptr<Type> typeUnion;
extern shared_ptr<Type> typeTimer;
extern shared_ptr<Type> typeTimerMgr;
extern shared_ptr<Type> typeVector;
}
}

#endif
