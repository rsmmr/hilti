#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
# @TEST-KNOWN-FAILURE:
#

event bro_init() &priority = 42
	{
	print set();
	print set(1,2,3);
	}
