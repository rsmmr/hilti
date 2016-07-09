
#include "../hilti-intern.h"

#include "optypes.h"

namespace hilti {
namespace optype {

shared_ptr<Type> any;

shared_ptr<Type> integer;
shared_ptr<Type> int8;
shared_ptr<Type> int16;
shared_ptr<Type> int32;
shared_ptr<Type> int64;

shared_ptr<Type> address;
shared_ptr<Type> bitset;
shared_ptr<Type> boolean;
shared_ptr<Type> bytes;
shared_ptr<Type> caddr;
shared_ptr<Type> callable;
shared_ptr<Type> channel;
shared_ptr<Type> classifier;
shared_ptr<Type> context;
shared_ptr<Type> double_;
shared_ptr<Type> enum_;
shared_ptr<Type> exception;
shared_ptr<Type> file;
shared_ptr<Type> function;
shared_ptr<Type> hook;
shared_ptr<Type> interval;
shared_ptr<Type> iosource;
shared_ptr<Type> label;
shared_ptr<Type> list;
shared_ptr<Type> map;
shared_ptr<Type> matchTokenState;
shared_ptr<Type> network;
shared_ptr<Type> overlay;
shared_ptr<Type> port;
shared_ptr<Type> regexp;
shared_ptr<Type> scope;
shared_ptr<Type> set;
shared_ptr<Type> string;
shared_ptr<Type> struct_;
shared_ptr<Type> time;
shared_ptr<Type> timer;
shared_ptr<Type> timerMgr;
shared_ptr<Type> tuple;
shared_ptr<Type> union_;
shared_ptr<Type> vector;

shared_ptr<Type> iterBytes;
shared_ptr<Type> iterIOSource;
shared_ptr<Type> iterList;
shared_ptr<Type> iterMap;
shared_ptr<Type> iterSet;
shared_ptr<Type> iterVector;

shared_ptr<Type> refAny;
shared_ptr<Type> refBytes;
shared_ptr<Type> refCallable;
shared_ptr<Type> refChannel;
shared_ptr<Type> refClassifier;
shared_ptr<Type> refContext;
shared_ptr<Type> refException;
shared_ptr<Type> refFile;
shared_ptr<Type> refIOSource;
shared_ptr<Type> refList;
shared_ptr<Type> refMap;
shared_ptr<Type> refMatchTokenState;
shared_ptr<Type> refOverlay;
shared_ptr<Type> refRegExp;
shared_ptr<Type> refSet;
shared_ptr<Type> refStruct;
shared_ptr<Type> refTimer;
shared_ptr<Type> refTimerMgr;
shared_ptr<Type> refVector;

shared_ptr<Type> typeAny;
shared_ptr<Type> typeBytes;
shared_ptr<Type> typeCallable;
shared_ptr<Type> typeChannel;
shared_ptr<Type> typeClassifier;
shared_ptr<Type> typeException;
shared_ptr<Type> typeFile;
shared_ptr<Type> typeIOSource;
shared_ptr<Type> typeList;
shared_ptr<Type> typeMap;
shared_ptr<Type> typeMatchTokenState;
shared_ptr<Type> typeOverlay;
shared_ptr<Type> typeRegExp;
shared_ptr<Type> typeSet;
shared_ptr<Type> typeStruct;
shared_ptr<Type> typeUnion;
shared_ptr<Type> typeTimer;
shared_ptr<Type> typeTimerMgr;
shared_ptr<Type> typeVector;

void __init()
{
    any = std::make_shared<type::Any>();

    integer = std::make_shared<type::Integer>();
    int8 = std::make_shared<type::Integer>(8);
    int16 = std::make_shared<type::Integer>(16);
    int32 = std::make_shared<type::Integer>(32);
    int64 = std::make_shared<type::Integer>(64);

    address = std::make_shared<type::Address>();
    bitset = std::make_shared<type::Bitset>();
    boolean = std::make_shared<type::Bool>();
    bytes = std::make_shared<type::Bytes>();
    caddr = std::make_shared<type::CAddr>();
    callable = std::make_shared<type::Callable>();
    channel = std::make_shared<type::Channel>();
    classifier = std::make_shared<type::Classifier>();
    context = std::make_shared<type::Context>();
    double_ = std::make_shared<type::Double>();
    enum_ = std::make_shared<type::Enum>();
    exception = std::make_shared<type::Exception>();
    file = std::make_shared<type::File>();
    function = std::make_shared<type::HiltiFunction>();
    hook = std::make_shared<type::Hook>();
    interval = std::make_shared<type::Interval>();
    iosource = std::make_shared<type::IOSource>();
    label = std::make_shared<type::Label>();
    list = std::make_shared<type::List>();
    map = std::make_shared<type::Map>();
    matchTokenState = std::make_shared<type::MatchTokenState>();
    network = std::make_shared<type::Network>();
    overlay = std::make_shared<type::Overlay>();
    port = std::make_shared<type::Port>();
    regexp = std::make_shared<type::RegExp>();
    scope = std::make_shared<type::Scope>();
    set = std::make_shared<type::Set>();
    string = std::make_shared<type::String>();
    struct_ = std::make_shared<type::Struct>();
    time = std::make_shared<type::Time>();
    timer = std::make_shared<type::Timer>();
    timerMgr = std::make_shared<type::TimerMgr>();
    tuple = std::make_shared<type::Tuple>();
    union_ = std::make_shared<type::Union>();
    vector = std::make_shared<type::Vector>();

    iterBytes = std::make_shared<type::iterator::Bytes>();
    iterIOSource = std::make_shared<type::iterator::IOSource>();
    iterList = std::make_shared<type::iterator::List>(std::make_shared<type::List>());
    iterMap = std::make_shared<type::iterator::Map>(std::make_shared<type::Map>());
    iterSet = std::make_shared<type::iterator::Set>(std::make_shared<type::Set>());
    iterVector = std::make_shared<type::iterator::Vector>(std::make_shared<type::Vector>());

    refAny = std::make_shared<type::Reference>();
    refBytes = std::make_shared<type::Reference>(bytes);
    refCallable = std::make_shared<type::Reference>(callable);
    refChannel = std::make_shared<type::Reference>(channel);
    refClassifier = std::make_shared<type::Reference>(classifier);
    refContext = std::make_shared<type::Reference>(context);
    refException = std::make_shared<type::Reference>(exception);
    refFile = std::make_shared<type::Reference>(file);
    refIOSource = std::make_shared<type::Reference>(iosource);
    refList = std::make_shared<type::Reference>(list);
    refMap = std::make_shared<type::Reference>(map);
    refMatchTokenState = std::make_shared<type::Reference>(matchTokenState);
    refOverlay = std::make_shared<type::Reference>(overlay);
    refRegExp = std::make_shared<type::Reference>(regexp);
    refSet = std::make_shared<type::Reference>(set);
    refStruct = std::make_shared<type::Reference>(struct_);
    refTimer = std::make_shared<type::Reference>(timer);
    refTimerMgr = std::make_shared<type::Reference>(timerMgr);
    refVector = std::make_shared<type::Reference>(vector);

    typeAny = std::make_shared<type::TypeType>();
    typeBytes = std::make_shared<type::TypeType>(bytes);
    typeCallable = std::make_shared<type::TypeType>(callable);
    typeChannel = std::make_shared<type::TypeType>(channel);
    typeClassifier = std::make_shared<type::TypeType>(classifier);
    typeException = std::make_shared<type::TypeType>(exception);
    typeFile = std::make_shared<type::TypeType>(file);
    typeIOSource = std::make_shared<type::TypeType>(iosource);
    typeList = std::make_shared<type::TypeType>(list);
    typeMap = std::make_shared<type::TypeType>(map);
    typeMatchTokenState = std::make_shared<type::TypeType>(matchTokenState);
    typeOverlay = std::make_shared<type::TypeType>(overlay);
    typeRegExp = std::make_shared<type::TypeType>(regexp);
    typeSet = std::make_shared<type::TypeType>(set);
    typeStruct = std::make_shared<type::TypeType>(struct_);
    typeUnion = std::make_shared<type::TypeType>(union_);
    typeTimer = std::make_shared<type::TypeType>(timer);
    typeTimerMgr = std::make_shared<type::TypeType>(timerMgr);
    typeVector = std::make_shared<type::TypeType>(vector);
}
}
}
