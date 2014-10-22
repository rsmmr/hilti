#
# @TEST-EXEC: bro -r ${TRACES}/bacnet/BBMD_Results.pcap bacnet.evt ${PAC2}/bacnet.bro %INPUT >output
# @TEST-EXEC: btest-diff output
#

event bacnet_message(c: connection, is_orig: bool, func: BACnet::BVLC_function, len: count)
  {
  print "Message", c$id$orig_h, c$id$resp_h, func, len;
  }

event bacnet_bvlc_result(c: connection, result: BACnet::BVLC_result)
  {
  print "Bvlc result", c$id$orig_h, c$id$resp_h, result;
  }

event bacnet_bvlc_write_bdt(c: connection, result: vector of BACnet::BDT_entry)
  {
  print "Write bdt", c$id$orig_h, c$id$resp_h, result;
  }

event bacnet_bvlc_read_bdt_ack(c: connection, result: vector of BACnet::BDT_entry)
  {
  print "Bdt ack", c$id$orig_h, c$id$resp_h, result;
  }

event bacnet_register_fd(c: connection, ttl: count)
  {
  print "Register fd", c$id$orig_h, c$id$resp_h, ttl;
  }

event bacnet_bvlc_read_fdt_ack(c: connection, result: vector of BACnet::FDT_entry)
  {
  print "Fdt ack", c$id$orig_h, c$id$resp_h, result;
  }

event bacnet_bvlc_delete_ftd_entry(c: connection, result: vector of BACnet::FDT_entry)
  {
  print "Delete ftd", c$id$orig_h, c$id$resp_h, result;
  }

event bacnet_bvlc_forwarded_npdu_information(c: connection, originator: addr, originator_port: count)
  {
  print "Forwarded npdu", c$id$orig_h, c$id$resp_h, originator, originator_port;
  }
