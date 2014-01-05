#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local a = 100;
	local b = 100.0;
	local c = 100secs;

	a -= 10;
	b -= 10;
	c -= 10secs;

	print a;
	print b;
	print c;
	}
