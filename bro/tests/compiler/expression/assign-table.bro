#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local x: table[string] of count;
	local y: table[string, string] of count;

	x["a"] = 1;
	x["b"] = 2;
	print x;
	
	y["a", "A"] = 1;
	y["b", "B"] = 2;
	print y;
	}
