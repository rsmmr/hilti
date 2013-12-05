# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	print fmt("%s %d", "foo", 42);
	print cat(1, 2.0, 3secs);
	}
