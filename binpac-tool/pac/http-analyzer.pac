# $Id:$

%extern{
#include <ctype.h>
#include <bptool.h>

%}

# Remember to call bytestring::free() on the result.
function to_upper(s: const_bytestring): bytestring
	%{
	char* buf = new char[s.length() + 1];
	const char* sp = (const char*) s.begin();

	for ( int i = 0; i < s.length(); ++i )
		if ( islower(sp[i]) )
			buf[i] = toupper(sp[i]);
		else
			buf[i] = sp[i];

	buf[s.length()] = '\0';

	return bytestring((uint8*) buf, s.length());
	%}

connection HTTP_Conn() {
	upflow = HTTP_Flow(true);
	downflow = HTTP_Flow(false);
};

flow HTTP_Flow(is_orig: bool) {
	flowunit = HTTP_PDU(is_orig) withcontext (connection, this);

	# States.
	%member{
		int content_length_;
		DeliveryMode delivery_mode_;
		bytestring end_of_multipart_;

		double msg_start_time_;
		int msg_begin_seq_;
		int msg_header_end_seq_;
	%}

	%init{
		content_length_ = 0;
		delivery_mode_ = UNKNOWN_DELIVERY_MODE;

		msg_start_time_ = 0;
		msg_begin_seq_ = 0;
		msg_header_end_seq_ = -1;
	%}

	%cleanup{
		end_of_multipart_.free();
	%}

	function content_length(): int
		%{
		return content_length_;
		%}

	function delivery_mode(): DeliveryMode
		%{
		return delivery_mode_;
		%}

	function end_of_multipart(): const_bytestring
		%{
		return end_of_multipart_;
		%}

	# Methods.
	function http_request(method: const_bytestring, uri: const_bytestring,
				vers: HTTP_Version): bool
		%{
        if ( bptool_verbose )
            cout << "request " << std_str(method) << " | " << std_str(uri) << " | " << std_str(vers->vers_str()) << endl;

		http_message_begin();

		return true;
		%}

	function http_reply(vers: HTTP_Version, code: int,
				reason: const_bytestring): bool
		%{
        if ( bptool_verbose )
            cout << "reply   " << std_str(vers->vers_str()) << " | " << code << " | " << std_str(reason) << endl;

		http_message_begin();

		return true;
		%}

	function extract_boundary(value: const_bytestring): bytestring
		%{
		const char* boundary_prefix = "boundary=";
		const char* boundary_begin = strcasestr(
						(const char*) value.begin(),
						boundary_prefix);

		if ( ! boundary_begin )
			return bytestring();

		boundary_begin += 9;

		const char* boundary_end = strcasestr(boundary_begin, ";");
		if ( ! boundary_end )
			boundary_end = (const char*) value.end();

		return bytestring((const uint8*) boundary_begin,
					(const uint8*) boundary_end);
		%}

	function is_end_of_multipart(line: const_bytestring): bool
		%{
		if ( line.length() < 4 + end_of_multipart_.length() )
			return false;

		int len = end_of_multipart_.length();

		// line =?= "--" end_of_multipart_ "--"
		return ( line[0] == '-' && line[1] == '-' &&
			 line[len + 2] == '-' && line[len + 3] == '-' &&
			 strncmp((const char*) line.begin() + 2,
				(const char*) end_of_multipart_.begin(),
				len) == 0 );
		%}

	function http_header(name_colon: const_bytestring,
	                     value: const_bytestring): bool
		%{
		const_bytestring name(
			name_colon.begin(),
			name_colon.length() > 0 ?
				name_colon.end() - 1 :
				name_colon.end());

		if ( bytestring_casecmp(name, "CONTENT-LENGTH") == 0 )
			{
			content_length_ = bytestring_to_int(value, 10);
			delivery_mode_ = CONTENT_LENGTH;
			}

		else if ( bytestring_casecmp(name, "TRANSFER-ENCODING") == 0 )
			{
			if ( bytestring_caseprefix(value, "CHUNKED") )
				delivery_mode_ = CHUNKED;
			}

		else if ( bytestring_casecmp(name, "CONTENT-TYPE") == 0 )
			{
			if ( bytestring_caseprefix(value, "MULTIPART") )
				{
				end_of_multipart_.free();
				end_of_multipart_ = extract_boundary(value);
				if ( end_of_multipart_.length() > 0 )
					delivery_mode_ = MULTIPART;
				}
			}

		return true;
		%}

	function gen_http_all_headers(): void
		%{
		%}

	function http_end_of_headers(headers: HTTP_Headers): bool
		%{
		// Check if this is the first set of headers
		// (i.e. not headers after chunks).
		if ( msg_header_end_seq_ == -1 )
			msg_header_end_seq_ = flow_buffer_->data_seq();

		return true;
		%}

	function http_message_begin(): void
		%{
		msg_start_time_ = 1.0;
		%}

	function http_message_done(pdu: HTTP_PDU): bool
		%{
		end_of_multipart_.free();

		// Initialize for next message.
		msg_begin_seq_ = flow_buffer_->data_seq();
		msg_header_end_seq_ = -1;

		return true;
		%}
};

refine typeattr HTTP_RequestLine += &let {
	process_request: bool =
		$context.flow.http_request(method, uri, version);
};

refine typeattr HTTP_ReplyLine += &let {
	process_reply: bool =
		$context.flow.http_reply(version, status.stat_num, reason);
};

refine typeattr HTTP_Header += &let {
	process_header: bool =
		$context.flow.http_header(name, value);
};

refine typeattr HTTP_Headers += &let {
	process_end_of_headers: bool =
		$context.flow.http_end_of_headers(this);
};

refine typeattr HTTP_PDU += &let {
	process_message: bool =
		$context.flow.http_message_done(this);
};
