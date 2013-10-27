
event bro_init() &priority = 42
	{
	# table([1] = "Foo1", [2] = "Foo2")
	print Hilti::test_type_table(Hilti::test_type_table2());
	}
	
