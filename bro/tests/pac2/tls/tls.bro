#
# @TEST-EXEC: bro -b -r ${TRACES}/tls/tls1.2.trace tls.evt ${PAC2}/tls.bro %INPUT >tls1.2
# @TEST-EXEC: btest-diff tls1.2
#
# @TEST-EXEC: bro -b -r ${TRACES}/tls/tls-conn-with-extensions.trace tls.evt ${PAC2}/tls.bro %INPUT >tls-extensions
# @TEST-EXEC: btest-diff tls-extensions
#
# @TEST-EXEC: bro -b -r ${TRACES}/tls/google.pcap tls.evt ${PAC2}/tls.bro %INPUT >google
# @TEST-EXEC: btest-diff google
#

event tls_client_hello(c: connection, version: TLS::TLSVersion, possible_ts: count, client_random: string, session_id: string, ciphers: vector of count)
  {
  print "==Client hello";
  print c$id$orig_h, c$id$resp_h;
  print "Version", version;
  print "ts", possible_ts;
  print "client random", client_random;
  print "session id", session_id;
  print "cipher list", ciphers;
  print "==Client hello end";
  }

event tls_server_hello(c: connection, version: TLS::TLSVersion, possible_ts: count, server_random: string, session_id: string, cipher: count, comp_method: count)
  {
  print "==Server hello";
  print c$id$orig_h, c$id$resp_h;
  print "Version", version;
  print "ts", possible_ts;
  print "server random", server_random;
  print "session id", session_id;
  print "cipher", cipher;
  print "compression metod", comp_method;
  print "==Server hello end";
  }

event tls_session_ticket_handshake(c: connection, ticket_lifetime_hint: count, ticket: string)
  {
  print "session ticket handshake", ticket_lifetime_hint, ticket;
  }

event tls_extension_unknown(c: connection, is_orig: bool, code: count, extension_type: TLS::Extensions, val: string)
  {
  print "unknown extension", c$id$orig_h, c$id$resp_h, code, extension_type, val;
  }

event tls_extension_next_protocol_negotiation(c: connection, is_orig: bool)
  {
  print "tls_extension_next_protocol_negotiation", c$id$orig_h, c$id$resp_h;
  }

event tls_extension_ec_point_formats(c: connection, is_orig: bool, points: vector of count)
  {
  print "tls_extension_ec_point_formats", c$id$orig_h, c$id$resp_h;
  }

event tls_extension_elliptic_curves(c: connection, is_orig: bool, curves: vector of count)
  {
  print "tls_extension_elliptic_curves", c$id$orig_h, c$id$resp_h, curves;
  }

event tls_extension_sessionticket_tls(c: connection, is_orig: bool, ticket_data: string)
  {
  print "tls_extension_sessionticket_tls", c$id$orig_h, c$id$resp_h, ticket_data;
  }

event tls_extension_heartbeat(c: connection, is_orig: bool, mode: TLS::HeartbeatMode)
  {
  print "tls_extension_heartbeat", c$id$orig_h, c$id$resp_h, mode;
  }

event tls_extension_signature_algorithms(c: connection, is_orig: bool, algorithms: vector of TLS::SignatureAndHashAlgorithm)
  {
  print "tls_extension_signature_algorithms", c$id$orig_h, c$id$resp_h, algorithms;
  }

event tls_extension_renegotiation_info(c: connection, is_orig: bool, info: string)
  {
  print "tls_extension_renegotiation_info", c$id$orig_h, c$id$resp_h, info;
  }

event tls_extension_server_name(c: connection, is_orig: bool, names: vector of string)
  {
  print "tls_extension_server_name", c$id$orig_h, c$id$resp_h, names;
  }

event tls_extension_application_layer_protocol_negotiation(c: connection, is_orig: bool, protocols: vector of string)
  {
  print "tls_extension_application_layer_protocol_negotiation", c$id$orig_h, c$id$resp_h, protocols;
  }

event tls_extension_status_request(c: connection, is_orig: bool, responder_id_list: string, request_extensions: string)
  {
  print "tls_extension_status_request", c$id$orig_h, c$id$resp_h, responder_id_list, request_extensions;
  }

event tls_extension_signed_certificate_timestamp(c: connection, is_orig: bool, signed_certificate_timestamp: string)
  {
  print "tls_extension_signed_certificate_timestamp", c$id$orig_h, c$id$resp_h, signed_certificate_timestamp;
  }
