#
# @TEST-EXEC: bro ./dtest.evt %INPUT
#
# @TEST-KNOWN-FAILURE: using the same enum twice in different events results in an error.

# @TEST-START-FILE dtest.evt

grammar dtest.spicy;

protocol analyzer spicy::dtest over UDP:
    parse with dtest::Message,
    port 47808/udp;

on dtest::Message if ( self.sswitch == 0 )
  -> event dtest_one(self.result);

on dtest::Message if ( self.sswitch == 1 )
  -> event dtest_two(self.result);

# @TEST-END-FILE
# @TEST-START-FILE dtest.spicy

module dtest;

type RESULT = enum {
 A, B, C, D, E, F
};

export type Message = unit {
  sswitch: uint8;
  result: uint8 &convert=RESULT($$);
};

# @TEST-END-FILE
