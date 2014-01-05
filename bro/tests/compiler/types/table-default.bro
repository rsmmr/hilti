#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

global TTX: table[string] of count &default=42;

event bro_init() &priority = 42
	{
	TTX["xyz"] = 21;
	print TTX["xyz"];
	print TTX["abc"];
	}



