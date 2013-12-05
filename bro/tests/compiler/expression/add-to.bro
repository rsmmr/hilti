#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local a = 10;
	local b = 10.0;
	local c = 10secs;
	local d = "Foo";

	a += 10;
	b += 10;
	c += 10secs;
	d += "Bar";

	print a;
	print b;
	print c;
	print d;
	}
