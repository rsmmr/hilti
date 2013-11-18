#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local x: count;
	local y = 2;

	x = 1;
	print x;
	print y;
	}
