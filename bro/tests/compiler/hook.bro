#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#


hook myhook(a: count) &priority=1
	{
	print "1", a;
	}

hook myhook(a: count) &priority=2
	{
	print "2", a;
	if ( a == 1 ) break;
	}

hook myhook(a: count) &priority=3
	{
	print "3", a;
	}

event bro_init()
	{
	print hook myhook(1);
	print hook myhook(2);
	}

