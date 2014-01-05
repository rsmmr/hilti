#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

type Foo: record  {
	a: count;
	b: string &default="DeFaUlT";
	c: double &optional;
};

type Bar: record  {
	a: count;
	b: string &default="DeFaUlT";
};

event bro_init() &priority = 42
	{
	local f: Foo;
	local b: Bar = [$a=42, $b="new"];

	f = [$a=42, $b="new"];
	print f;
	
	f = b;
	print f;
	}
