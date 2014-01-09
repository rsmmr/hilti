
function foo(n: count) : count
	{
	if ( n < 2 )
		return n;

	return foo(n - 1) + foo(n - 2);
	}

event bro_init() &priority = 42
	{
	print "- begin";
	print foo(35);
	print "- done";
	}

