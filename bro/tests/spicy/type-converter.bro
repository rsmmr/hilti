#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./conv.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

@TEST-START-FILE conv.spicy

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

    var s: set< uint<64> > = set< uint<64> >(1,2,3);
    var v: vector<bytes> = vector<bytes>(b"A", b"B", b"C");
    var l: list<bytes> = list<bytes>(b"A", b"B", b"C");

    on %done { print self; }
};

@TEST-END-FILE


@TEST-START-FILE conv.evt

grammar conv;

protocol analyzer Conv over TCP:
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
                                  self.h,
				  self.s,
				  self.v,
				  self.l
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
                 h: string,
		 s: set[count],
		 v: vector of string,
		 l: vector of string
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
	print s;
	print v;
	print l;
	}

