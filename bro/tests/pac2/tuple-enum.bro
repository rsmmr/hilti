#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./tupleenum.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#
# @TEST-KNOWN-FAILURE: binpac++ enum values do not work in bro records
#
# Error with test as it is:
# no location>:: error, type enum { A = 83, B = 84, C = 85 } is not compatible with target type GLOBAL::enum_0x7fac1730d390 [pass::hilti::Validator]
#
# Error when commenting the event line in tupleenum.evt:
# error in [..]/tuple-enum.bro, line 9: identifier not defined: TupleEnum::TestEnum

type Foo: record {
    i: TupleEnum::TestEnum;
    j: count;
};

event enum_message(f: Foo) {
    print f;
}

# @TEST-START-FILE tupleenum.evt

grammar tupleenum.pac2;

protocol analyzer TupleEnum over TCP:
    parse with TupleEnum::Message,
    port 22/tcp,
    replaces SSH;

on TupleEnum::Message -> event enum_message( (self.a, cast<uint64>(self.b)) );

# @TEST-END-FILE

# @TEST-START-FILE tupleenum.pac2

module TupleEnum;

type TestEnum = enum {
    A = 83, B = 84, C = 85
};

export type Message = unit {
    a: uint8 &convert=TestEnum($$);
    b: uint8; 

    on %done { print self; }
};

# @TEST-END-FILE
