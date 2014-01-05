#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output 2>&1
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local t1 = table([1] = "A", [2] = "B");
	print "A";
	print t1[3];
	print "B";
	}
