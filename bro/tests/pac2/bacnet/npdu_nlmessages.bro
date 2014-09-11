#
# @TEST-EXEC: bro -r ${TRACES}/bacnet/NPDU.pcap bacnet.evt ${PAC2}/bacnet.bro %INPUT >output
# @TEST-EXEC: btest-diff output

event bacnet_npdu_network_messages(c: connection, info: BACnet::NPDU_info, dnets: vector of count)
  {
  print "network message", c$id$orig_h, c$id$resp_h, info, dnets;
  }

event bacnet_npdu_i_could_be_router_to_network(c: connection, info: BACnet::NPDU_info, dnets: vector of count, performance_index: count)
  {
  print "i could be router to network", c$id$orig_h, c$id$resp_h, info, dnets, performance_index;
  }

event bacnet_npdu_reject_message_to_network(c: connection, info: BACnet::NPDU_info, dnets: vector of count, reason: BACnet::NPDU_Reject_reason)
  {
  print "reject message to network", c$id$orig_h, c$id$resp_h, info, dnets, reason;
  }

event bacnet_npdu_what_is_network_number(c: connection, info: BACnet::NPDU_info)
  {
  print "what is network number", c$id$orig_h, c$id$resp_h, info;
  }

event bacnet_npdu_network_number_is(c: connection, info: BACnet::NPDU_info, network_number: count, learned: count)
  {
  print "network number is", c$id$orig_h, c$id$resp_h, info, network_number, learned;
  }

event bacnet_npdu_routing_table_change(c: connection, info: BACnet::NPDU_info, entries: vector of BACnet::NPDU_routing_entry)
  {
  print "routing table change", c$id$orig_h, c$id$resp_h, info, entries;
  }

event bacnet_npdu_establish_connection_to_network(c: connection, info: BACnet::NPDU_info, dnets: vector of count, termination_time: count)
  {
  print "establish connection to network", c$id$orig_h, c$id$resp_h, info, dnets, termination_time;
  }
