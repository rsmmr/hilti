#
# @TEST-EXEC: bro -r ${TRACES}/ssh-single-conn.trace ./listconv.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

@TEST-START-FILE listconv.pac2

module listconv;

tuple<uint64, uint64>
bro_convert(a: uint8)
{
    return (a, a + 1);
}


export type Test = unit {
    a: list<uint8> &length=5;
    b: int<16>;
    c: uint<16>;
};

@TEST-END-FILE


@TEST-START-FILE listconv.evt

grammar listconv;

protocol analyzer listconv over TCP:
    parse originator with listconv::Test,
    port 22/tcp;

on listconv::Test -> event listconv::test($conn,
                                  $is_orig,
                                  [ listconv::bro_convert(i) for i in self.a ],
                                  self.b,
                                  self.c
                                  );

@TEST-END-FILE

type int_tuple: record {
  a: count;
  b: count;
};

event listconv::test(x: connection,
                 is_orig: bool,
                 a: set[int_tuple],
                 b: int,
                 c: count
                ) {
  print x$id;
  print is_orig;
  print a;
  print b;
  print c;
}

