
event bro_init() &priority = 42
	{
	print "Hallo!";
	print Hilti::is_compiled();
	print Hilti::test_type_string("foo");
	}
	
