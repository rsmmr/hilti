#
# @TEST-EXEC: HILTI_DEBUG=hilti-trace:hilti-flow bro -r ${TRACES}/ssh-single-conn.trace -b %INPUT Hilti::compile_scripts=T Hilti::debug=T >output
# @TEST-EXEC: btest-diff output
# @TEST-EXEC: grep -m 1 -A 6 "call LibBro::object_mapping_lookup_bro" hlt-debug.log >debug
# @TEST-EXEC: btest-diff debug
#
# Check that passing the connection record back through Bro uses the object
# caching.

function foo(c: connection): string
	{
        return fmt("-> %s", c$id$resp_h);
	}

event connection_established(c: connection)
	{
	local f = foo;
	local s = f(c);
	s = f(c);
	print s;
	}






