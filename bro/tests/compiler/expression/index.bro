#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local t1 = table([1] = "A", [2] = "B");
	print t1[1];

	local t2 = table([1,2] = "AA", [2, 3] = "BC");
	print t2[1,2];

	print "";

	###

	local v = vector("A", "B", "C");
	print v[1];

	print "";

	### From Bro's string-indexing.bro

	local s = "0123456789";

	print s[1];
	print s[1:2];
	print s[1:6];
	print s[0:20];
	print s[-2];
	print s[-3:-1];
	print s[-1:-10];
	print s[-1:0];
	print s[-1:5];
	print s[20:23];
	print s[-20:23]; #
	print s[0:5][2];
	print s[0:5][1:3][0];
	}
