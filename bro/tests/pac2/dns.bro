#
# @TEST-EXEC: bro -r ${TRACES}/dns.trace dns.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

event my_dns_request(c: connection, msg: dns_msg, query: string, qtype: count, qclass: count)
	{
	print c$id, msg, query, qtype, qclass;
	}
