
module BACnet;

export {
  type BDT_entry: record {
    address: addr;
    address_port: count;
    mask: count;
  };

  type FDT_entry: record {
    address: addr;
    address_port: count;
    ttl: count;
    time_remaining: count;
  };

  type NPDU_info: record {
    priority: NPDU_priority;
    dnet: count &optional;  # destination network number. FFFF = global broadcast.
    dadr: string &optional; # destination mac layer address, variable length
    snet: count &optional;  # same as destination
    sadr: string &optional; 
    hop_count: count &optional; # decrementinghop-count. Initialized with 255
    message_type: NPDU_type &optional; # NL message type if NPDU contains nl-message
  };

  type NPDU_routing_entry: record {
    dnet: count;
    portId: count;
    portInfo: string;
  };

  type NPDU_initialize_routing_table: record {
    number_of_ports: count;
    mappings: vector of NPDU_routing_entry;
  };
}
