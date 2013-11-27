#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local x = table([1] = "A", [2] = "B");
	local y = table([1,2,3] = "A", [2,3,4] = "B");

	delete x[1];
	delete x[2];
	print x;

	delete y[1,2,3];
	print y;
	}

