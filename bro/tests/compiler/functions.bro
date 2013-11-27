#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

function foo1()
	{
	print "foo1";
	}

function foo2() : string
	{
	return "foo2";
	}

function foo3(s: string)
	{
	print s;
	}

function foo4(i: count, s: string &default="foo4")
	{
	print s, i;
	}

event bro_init() &priority = 42
	{
	foo1();
	print foo2();
	foo3("foo3");
	foo4(42);
	}




