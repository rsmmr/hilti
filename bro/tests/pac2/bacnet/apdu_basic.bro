#
# @TEST-EXEC: bro -r ${TRACES}/bacnet/rpm.pcap bacnet.evt ${PAC2}/bacnet.bro %INPUT >rpm-out
# @TEST-EXEC: btest-diff rpm-out
# @TEST-EXEC: bro -r ${TRACES}/bacnet/who-is-i-am.pcap bacnet.evt ${PAC2}/bacnet.bro %INPUT >who-out
# @TEST-EXEC: btest-diff who-out

event bacnet_apdu_confirmed_request(c: connection, invokeID: count, service: BACnet::BACnetConfirmedServiceChoice)
  {
  print "confirmed request", c$id$orig_h, c$id$resp_h, invokeID, service;
  }

event bacnet_apdu_unconfirmed_request(c: connection, service: BACnet::BACnetUnconfirmedServiceChoice)
  {
  print "unconfirmed request", c$id$orig_h, c$id$resp_h, service;
  }

event bacnet_apdu_simple_ack(c: connection, invokeID: count, service: BACnet::BACnetConfirmedServiceChoice)
  {
  print "simple ack", c$id$orig_h, c$id$resp_h, invokeID, service;
  }

event bacnet_apdu_complex_ack(c: connection, invokeID: count, service: BACnet::BACnetConfirmedServiceChoice)
  {
  print "complex ack", c$id$orig_h, c$id$resp_h, invokeID, service;
  }

event bacnet_apdu_segment_ack(c: connection, invokeID: count, sequence_number: count, actual_window_size: count)
  {
  print "segment ack", c$id$orig_h, c$id$resp_h, invokeID, sequence_number, actual_window_size;
  }

event bacnet_apdu_error(c: connection, invokeID: count, error_class: count, error_code: count)
  {
  print "error", c$id$orig_h, c$id$resp_h, invokeID, error_class, error_code;
  }

event bacnet_apdu_reject(c: connection, invokeID: count, reason: BACnet::BACnetRejectReason)
  {
  print "reject", c$id$orig_h, c$id$resp_h, invokeID, reason;
  }

event bacnet_apdu_abort(c: connection, invokeID: count, reason: BACnet::BACnetAbortReason)
  {
  print "abort", c$id$orig_h, c$id$resp_h, invokeID, reason;
  }
