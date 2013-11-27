#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

type Foo: record  {
	a: count;
	b: string &default="DeFaUlT";
	c: double &optional;
	d: bool;
};

event bro_init() &priority = 42
	{
	local f: Foo;

	f$d = T;
	
	print f?$a;
	print f?$b;
	print f?$c;
	print f?$d;
	}
