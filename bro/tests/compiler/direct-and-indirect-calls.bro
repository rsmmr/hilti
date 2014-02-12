#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T Hilti::save_hilti=T >output
# @TEST-EXEC: btest-diff output
#
# Make sure that the calls are indeed direct and indirect, respectively:
#
# @TEST-EXEC: cat bro.GLOBAL.hlt | sed -n '/^hook void bro_init/,/^}/p'| egrep -1 'call.*(foo|legacy)' >code
# @TEST-EXEC: cat bro.GLOBAL.hlt | egrep 'global.*_ctor_' >>code
# @TEST-EXEC: btest-diff code
#
#
function foo1(s: string) : count
	{
	print "foo1", s, Hilti::is_compiled();
	return 1;
	}

function foo2(s: string) : count
	{
	print "foo2", s, Hilti::is_compiled();
	return 2;
	}

event bro_init() &priority = 42
	{
	local c: count;
	local f: function(s: string) : count;

	c = foo1("direct call");
	print "Result", c;

	f = foo2;
	c = f("indirect call");
	print "Result", c;
	}





