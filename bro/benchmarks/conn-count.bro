
global counters: table[addr, addr] of count &default=0;
global total_conns: count = 0;

event connection_established(c: connection)
	{
	local orig = c$id$orig_h;
	local resp = c$id$resp_h;

	counters[orig,resp] += 1;
	++total_conns;
	}

event bro_done()
	{
	local c: count = 0;

	for ( [orig, resp] in counters )
		c += counters[orig, resp];

	print fmt("# compiled = %s", Hilti::is_compiled());
	print fmt("Total connections: %d/%d", total_conns, c);
	print fmt("Total pairs      : %d", |counters|);
	}

