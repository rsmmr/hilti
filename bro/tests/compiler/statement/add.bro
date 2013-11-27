#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local x: set[count];
	local y: set[count, string];

	add x[1];
	add x[2];
	print x;

	add y[1, "A"];
	add y[2, "B"];
	print y;
	}

