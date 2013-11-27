#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

type Foo: record  {
	a: count;
	b: string &default="DeFaUlT";
	c: double &optional;
};

event bro_init() &priority = 42
	{
	local f: Foo;

	print f;
	f$a = 42;
	f$b = "BBB";
	print f;
	}
