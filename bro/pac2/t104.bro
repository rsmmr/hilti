@load base/protocols/conn

global apci_counter = 0;
global tcp_apci_counter = 0;
global mode_i_counter = 0;
global mode_s_counter = 0;
global mode_u_counter = 0;

event bro_init() {
	print "bro_init";
}

event t104::apci (c: connection, len: int, mode: int, i_send_seq: int, u_start_dt: int, u_stop_dt: int, u_test_fr: int, recv_seq: int) {
	#print "APCI", c$id$orig_h, c$id$resp_h;#, "len", len,"mode", mode, "i_send_seq", i_send_seq,"u_start_dt", u_start_dt,"u_stop_dt", u_stop_dt,"u_test_fr", u_test_fr,"recv_seq", recv_seq;
	apci_counter = apci_counter + 1;
}

type cause:record {
	#cot : count;
    negative : bool;
    test : bool;
    #info_obj_type : string;
    common_addr : int;
    #cot : int;
};

event t104::asdu (c: connection, cot: cause, info_obj_type: T104::Info_obj_code, cause_of_transmission: T104::Cot_code) {
	print "ASDU", cot, info_obj_type, cause_of_transmission;
}

event t104::i (send_seq: count, recv_seq: count) {
	mode_i_counter += 1;
}

event t104::s (recv_seq: count) {
	mode_s_counter += 1;
}

event t104::u (u_start_dt: count, u_stop_dt: count, u_test_fr: count) {
	mode_u_counter += 1;
}

#event connection_established (c: connection){
#	print "TCP connection_established", c$id$orig_h, c$id$resp_h, c$id$orig_p, c$id$resp_p, c$service;
#}

#event connection_state_remove(c: connection) {
	#print "TCP connection_state_remove", c$id$orig_h, c$id$resp_h, c$id$orig_p, c$id$resp_p, c$service;
#}

event bro_done () {
	print "apci_counter", apci_counter;
	print "mode_u_counter", mode_u_counter;
	print "mode_s_counter", mode_s_counter;
	print "mode_i_counter", mode_i_counter;

}