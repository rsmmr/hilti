
module T104;

event t104::apci (c: connection, len: int, mode: int, i_send_seq: int, u_start_dt: int, u_stop_dt: int, u_test_fr: int, recv_seq: int) {
	print "WOHO", c$id$orig_h, c$id$resp_h, len, mode, i_send_seq, u_start_dt, u_stop_dt, u_test_fr, recv_seq;
}

type cause:record {
	#cot : count;
    negative : bool;
    test : bool;
};

event t104::asdu (c: connection, cot: cause) {
	print "ASDU", cot;
}
