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
	c1 = 10;
	c2 = 20;

	print i2 / i1;
	print c2 / c1;
	print 20.0 / 10.0;
	}

