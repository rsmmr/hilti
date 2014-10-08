#
# @TEST-EXEC: bro -C -r ${TRACES}/bacnet/who-has.pcap bacnet.evt ${PAC2}/bacnet.bro %INPUT >out
# @TEST-EXEC: btest-diff out

event bacnet_i_am_request(c: connection, tpe: BACnet::BACnetObjectType, instance: count, max_apdu_length_accepted: count, segmentation_supported: count, vendorID: BACnet::BACnetVendorIDs)
  {
  print "I-am", c$id$orig_h, c$id$resp_h, tpe, instance, max_apdu_length_accepted, segmentation_supported, vendorID;
  }

event bacnet_who_is_request(c: connection, low: count, high: count)
  {
  print "Who-is", c$id$orig_h, c$id$resp_h, low, high;
  }

event bacnet_who_has_request_device(c: connection, tpe: BACnet::BACnetObjectType, instance: count)
  {
  print "Who-has", c$id$orig_h, c$id$resp_h, tpe, instance;
  }

event bacnet_who_has_request_name(c: connection, name: string)
  {
  print "Who-has", c$id$orig_h, c$id$resp_h, name;
  }

event bacnet_i_have_request(c: connection, deviceIdentifier_type: BACnet::BACnetObjectType, deviceIdentifier_value: count, objectIdentifier_type: BACnet::BACnetObjectType, objectIdentifier_value: count, objectName: string)
  {
  print "I-have", c$id$orig_h, c$id$resp_h, deviceIdentifier_type, deviceIdentifier_value, objectIdentifier_type, objectIdentifier_value, objectName;
  }
