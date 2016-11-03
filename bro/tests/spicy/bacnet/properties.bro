#
# @TEST-EXEC: bro -C -r ${TRACES}/bacnet/read-properties.pcap bacnet.evt ${SPICY}/bacnet.bro %INPUT >out
# @TEST-EXEC: btest-diff out

event bacnet_readproperty_request(c: connection, invokeID: count, tpe: BACnet::BACnetObjectType, instance: count, property: BACnet::BACnetPropertyIdentifier)
  {
  print "Read", c$id$orig_h, c$id$resp_h, invokeID, tpe, instance, property;
  }

event bacnet_readproperty_ack(c: connection, invokeID: count, tpe: BACnet::BACnetObjectType, instance: count, property: BACnet::BACnetPropertyIdentifier)
  {
  print "ACK", c$id$orig_h, c$id$resp_h, invokeID, tpe, instance, property;
  }

event bacnet_apdu_error(c: connection, invokeID: count, service: BACnet::BACnetConfirmedServiceChoice, error_class: BACnet::BACnetErrorClass, error_code: BACnet::BACnetErrorCode)
  {
  print "Error", c$id$orig_h, c$id$resp_h, invokeID, service, error_class, error_code;
  }
