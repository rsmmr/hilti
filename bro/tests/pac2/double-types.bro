#
# @TEST-EXEC: bro ./dtest.evt %INPUT
#
# @TEST-START-FILE dtest.evt

grammar dtest.pac2;

protocol analyzer pac2::dtest over UDP:
    parse with dtest::Message,
    port 47808/udp;

on dtest::Message -> event dtest_message(self.func);

on dtest::Message -> event dtest_result(self.sub.result);

on dtest::Message -> event dtest_result_tuple(dtest::bro_result(self));

# @TEST-END-FILE
# @TEST-START-FILE dtest.pac2

module dtest;

export type FUNCS = enum {
 A=0, B=1, C=2, D=3, E=4, F=5
};

export type RESULT = enum {
 A, B, C, D, E, F
};

export type Message = unit {
  func: uint8 &convert=FUNCS($$);
  sub: SubMessage;
};

type SubMessage = unit {
  result: uint8 &convert=RESULT($$);
};

tuple<FUNCS, RESULT> bro_result(entry: Message) {
	return (entry.func, entry.sub.result);
}

# @TEST-END-FILE
