#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local i1: int;
	local i2: int;
	local c1: count;
	local c2: count;
	local t = double_to_time(1384581609.0);

	i1 = 10;
	i2 = 20;
	c1 = 20;
	c2 = 10;

	print i1 * i2;
	print c1 * c2;
	print 10.0 * 20.0;
	}

