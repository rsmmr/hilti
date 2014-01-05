#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event foo1() &priority=42
	{
	print "event foo1a";
	}

event foo1() &priority=42
	{
	print "event foo1b";
	}

event foo2(s: string) &priority=42
	{
	print "event foo2 with arg", s;
	}

event bro_init() &priority = 42
	{
	event foo1();
	event foo2("test");
	}




