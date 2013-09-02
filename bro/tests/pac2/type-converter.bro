#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./conv.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

@TEST-START-FILE conv.pac2

module Conv;

export type Test = unit {
    a: bytes &length=5;
    b: int<16>;
    c: uint<16>;
    d: bytes &length=1 &convert=3.14;
    e: bytes &length=1 &convert=1.2.3.4;
    f: bytes &length=1 &convert=[2001:0db8::1428:57ab];
    g: bytes &length=1 &convert=True;
    h: bytes &length=1 &convert="MyString";

    on %done { print self; }
};

@TEST-END-FILE


@TEST-START-FILE conv.evt

grammar conv;

analyzer Conv over TCP:
    parse originator with Conv::Test,
    port 22/tcp;

on Conv::Test -> event conv::test($conn,
                                  $is_orig,
                                  self.a,
                                  self.b,
                                  self.c,
                                  self.d,
                                  self.e,
                                  self.f,
                                  self.g,
                                  self.h
                                  );

@TEST-END-FILE


event conv::test(x: connection,
                 is_orig: bool,
                 a: string,
                 b: int,
                 c: count,
                 d: double,
                 e: addr,
                 f: addr,
                 g: bool,
                 h: string
                )
	{
	print x$id;
	print is_orig;
	print a;
	print b;
	print c;
	print d;
	print e;
	print f;
	print g;
	print h;
	}

