#
# @TEST-EXEC: HILTI_DEBUG=hilti-trace:hilti-flow bro -r ${TRACES}/ssh-single-conn.trace -b %INPUT Hilti::compile_scripts=T Hilti::debug=T >output
# @TEST-EXEC: btest-diff output
# @TEST-EXEC: grep -A 6 "call LibBro::object_mapping_lookup_bro" hlt-debug.log | awk '/^--/{f=1}; f==1 {exit} {print}' >debug
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






