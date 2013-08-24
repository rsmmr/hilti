
# Known difference:
#     - pac2 doesn't pass all the http_message_stat information.

module HTTPEvents;

export {
	global pac2: bool = T &redef;
	global log_events: bool = T &redef;
}

global out: file;

global entities: table[conn_id, bool] of string &default="";

event bro_init()
	{
	if ( log_events )
		{
		local prefix = pac2 ? "pac" : "std";
		out = open_log_file(fmt("http-events-%s", prefix));
		capture_events(fmt("events-%s.bst", prefix));
		}

	if ( pac2 )
		{
		Analyzer::disable_analyzer(Analyzer::ANALYZER_HTTP);
		Analyzer::enable_analyzer(Analyzer::ANALYZER_PAC2_HTTP);
		}
	else
		{
		Analyzer::enable_analyzer(Analyzer::ANALYZER_HTTP);
		Analyzer::disable_analyzer(Analyzer::ANALYZER_PAC2_HTTP);
		}
	}

event http_request(c: connection, method: string, original_URI: string, unescaped_URI: string, version: string)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_request", c$id, method, unescaped_URI, version;
	}

event http_reply(c: connection, version: string, code: count, reason: string)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_reply", c$id, version, code, reason;
	}

event http_header(c: connection, is_orig: bool, name: string, value: string)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_header", c$uid, is_orig, name, value;
	}

event http_message_done(c: connection, is_orig: bool, stat: http_message_stat)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_message_done", c$uid, is_orig, stat$body_length;
	}

event http_message_done_pac2(c: connection, is_orig: bool, body_length: count)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_message_done", c$uid, is_orig, body_length;
	}

event http_content_type(c: connection, is_orig: bool, ty: string, subty: string)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_content_type", c$uid, is_orig, ty, subty;
	}

event http_begin_entity(c: connection, is_orig: bool)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_begin_entities", c$uid, is_orig;

	delete entities[c$id, is_orig];
	}

event http_end_entity(c: connection, is_orig: bool)
	{
	if ( ! log_events )
		return;

	local d: string = "-";

	if ( [c$id, is_orig] in entities )
		d = fmt("|%s|", entities[c$id, is_orig]);

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "http_ALL_entities", c$uid, is_orig, d;
	print out, prefix, network_time(), "http_end_entities", c$uid, is_orig;

	delete entities[c$id, is_orig];

	}

event http_entity_data(c: connection, is_orig: bool, length: count, data: string)
	{
	if ( ! log_events )
		return;

	entities[c$id, is_orig] = entities[c$id, is_orig] + data;
	}

event get_file_handle(tag: Analyzer::Tag, c: connection, is_orig: bool)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "get_file_handle", c$uid, tag, is_orig;
	}

event file_new(f: fa_file)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "file_new", f$id, f$seen_bytes;
	}

event file_over_new_connection(f: fa_file, c: connection, is_orig: bool)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "file_over_new_connection", c$uid, f$id, f$seen_bytes, is_orig;
	}

event file_timeout(f: fa_file)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "file_timeout", f$id, f$seen_bytes;
	}

event file_gap(f: fa_file, offset: count, len: count)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "file_gap", f$id, f$seen_bytes, offset, len;
	}

event file_state_remove(f: fa_file)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "file_state_remove", f$id, f$seen_bytes;
	}

event protocol_confirmation(c: connection, atype: Analyzer::Tag, aid: count)
	{
	if ( ! log_events )
		return;

	local prefix = pac2 ? "pac" : "std";
	print out, prefix, network_time(), "protocol_confirmation", c$uid, atype, aid;
	}

