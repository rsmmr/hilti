#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

# TODO: Test user-defined enum type.
#       Test pre-defined and user defined record types.

event bro_init() &priority = 42
	{
	print HiltiTest::type_bool(T);
	print HiltiTest::type_int(-42);
	print HiltiTest::type_count(42);
	print HiltiTest::type_double(3.14);
	print HiltiTest::type_time(double_to_time(1384581609.0));
	print HiltiTest::type_interval(42secs);
	print HiltiTest::type_string("Foo");
	# print HiltiTest::type_pattern(/Foo/);  Not supported yet.
	print HiltiTest::type_enum(tcp);
	print HiltiTest::type_port(80/tcp);
	print HiltiTest::type_addr(1.2.3.4);
	print HiltiTest::type_subnet(2.3.4.5/27);
	print HiltiTest::type_table(table([1] = "foo", [2] = "bar"));
	print HiltiTest::type_set(set("aaa", "bbb", "ccc"));
	print HiltiTest::type_vector(vector("aaa", "bbb", "ccc"));
	print HiltiTest::type_record(conn_id($orig_h=1.2.3.4, $orig_p=1024/tcp, $resp_h=2.3.4.5, $resp_p=8080/tcp));
	}


