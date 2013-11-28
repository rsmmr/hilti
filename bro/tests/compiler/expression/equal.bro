#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	print 42 == 42;
	print 42 == 43;
	print 42 != 42;
	print 42 != 43;

	print "";
	
	print "foo" == "foo";
	print "foo" == "Foo";
	print "foo" != "foo";
	print "foo" != "Foo";
	}
