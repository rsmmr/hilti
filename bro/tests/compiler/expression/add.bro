#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	print 1 + 2;
	print "a" + "b";
	print 1.0 + 2.0;
	print 34sec + 26sec;
	}
