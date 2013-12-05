#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local a = 10;
	local t = table(["A"] = 10);

	print a;
	print ++a;
	print --a;
	print a;

	print "";
	
	print t["A"];
	print ++t["A"];
	print --t["A"];
	print t["A"];
	}
