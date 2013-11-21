#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	print table([1] = "A", [2] = "B");
	print table([1,2,3] = "A", [2,3,4] = "B");
	print table();
	}
