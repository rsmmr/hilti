#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local t = table([1] = "A", [2] = "B", [3] = "C");
	local s = set(1,2,3);
	local v  = vector(1,2,3);

	print |"abc"|;
	print |t|;
	print |s|;
	print |v|;
	

	}

