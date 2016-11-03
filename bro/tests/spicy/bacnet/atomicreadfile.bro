#
# @TEST-EXEC: bro -C -r ${TRACES}/bacnet/atomic-read-file.pcap bacnet.evt ${SPICY}/bacnet.bro %INPUT >out
# @TEST-EXEC: btest-diff out

event bacnet_atomicreadfile_request(c: connection, invokeID: count, stream: bool, tpe: BACnet::BACnetObjectType, instance: count, start: int, len: count)
  {
  print "Request", c$id$orig_h, c$id$resp_h, stream, invokeID, tpe, instance, start, len;
  }

event bacnet_atomicreadfile_ack(c: connection, invokeID: count, eof: bool, start: int, data: string)
  {
  print "Reply", c$id$orig_h, c$id$resp_h, invokeID, eof, start, |data|;
  }
