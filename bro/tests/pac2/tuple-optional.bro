#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./tupleoptional.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#
# @TEST-KNOWN-FAILURE: There is no way to have tuples/records with optional elements

# Slightly longer explanation - it would be super convenient to be able to have elements
# that remain (not set) in a unit passed to Bro as records with optional elements. This
# is especially true in protocols, that have optional fields, that are only set in certain
# circumstances (which seems to be rather common).
#
# This is only the proposal for a syntax. In this case, the tuple construction uses the .?
# operator, which either returns the actual element (if present), or sets the value as not-present
# in a way that is passed on to the bro record.

type Foo: record {
    i: int;
    j: int &optional;
    k: int &optional;
    s: string;
    ss: string &optional;
};

event tuple_message(f: Foo) {
    print f;
}

# @TEST-START-FILE tupleoptional.evt

grammar tupleoptional.pac2;

protocol analyzer TupleOptional over TCP:
    parse with TupleOptional::Message,
    port 22/tcp,
    replaces SSH;

on TupleOptional::Message -> event tuple_message( (cast<int64>(self.a), cast<int64>(self.?b), cast<int64>(self.?c), self.d, self.?e) );

# @TEST-END-FILE

# @TEST-START-FILE tupleoptional.pac2

module TupleOptional;

export type Message = unit {
    a: int8;
    b: int8; 
    c: int8 if ( 1 == 0 );
    d: bytes &length=2;
    e: bytes &length=1 if ( 1 == 0 );
};

# @TEST-END-FILE
