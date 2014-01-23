#
# @TEST-EXEC: bro ./dtest.evt %INPUT
#
# @TEST-KNOWN-FAILURE: enum convert problem. Comment line 16 to toggle error.

# @TEST-START-FILE dtest.evt

grammar dtest.pac2;

protocol analyzer pac2::dtest over UDP:
    parse with dtest::Message,
    port 47808/udp;

on dtest::Message -> event dtest_message(self.func);

on dtest::Message -> event dtest_result (self.sub.result);

# @TEST-END-FILE
# @TEST-START-FILE dtest.pac2

module dtest;

type FUNCS = enum {
 A, B, C, D, E, F
};

type RESULT = enum {
 A, B, C, D, E, F
};

export type Message = unit {
  func: uint8 &convert=FUNCS($$);
  sub: SubMessage;
};

type SubMessage = unit {
  result: uint8 &convert=RESULT($$);
};

# @TEST-END-FILE
