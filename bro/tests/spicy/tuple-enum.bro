#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./tupleenum.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

type Foo: record {
    i: TupleEnum::TestEnum;
    j: count;
};

event enum_message(f: Foo) {
	print f;
}

# @TEST-START-FILE tupleenum.evt

grammar tupleenum.spicy;

protocol analyzer TupleEnum over TCP:
    parse with TupleEnum::Message,
    port 22/tcp,
    replaces SSH;

on TupleEnum::Message -> event enum_message( (self.a, cast<uint64>(self.b)) );

# @TEST-END-FILE

# @TEST-START-FILE tupleenum.spicy

module TupleEnum;

export type TestEnum = enum {
    A = 83, B = 84, C = 85
};

export type Message = unit {
    a: uint8 &convert=TestEnum($$);
    b: uint8; 

    on %done { print self; }
};

# @TEST-END-FILE
