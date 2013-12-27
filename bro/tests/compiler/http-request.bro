#
# @TEST-EXEC: bro -b -r ${BROTRACES}/wikipedia.trace %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#
# 
const ports = {
	80/tcp, 81/tcp, 631/tcp, 1080/tcp, 3128/tcp,
	8000/tcp, 8080/tcp, 8888/tcp,
};

event bro_init() &priority=5
	{
	Analyzer::register_for_ports(Analyzer::ANALYZER_HTTP, ports);
	}

event http_request(c: connection, method: string, original_URI: string, unescaped_URI: string, version: string)
	{
	print "http_request", c, method, original_URI, unescaped_URI, version;
	}
