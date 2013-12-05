#
# @TEST-EXEC: bro -r ${TRACES}/dns.trace dns.evt %INPUT >output
# @TEST-EXEC: btest-diff output
#

event my_dns_request(c: connection, query: string, qtype: count, qclass: count)
	{
	print c$id, query, qtype, qclass;
	}
