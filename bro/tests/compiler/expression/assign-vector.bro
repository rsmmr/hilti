#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local x: vector of string;

	x[0] = "A";
	x[1] = "B";
	x[5] = "C";
	print x;
	}
