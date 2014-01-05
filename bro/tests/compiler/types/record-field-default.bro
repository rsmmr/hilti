#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

type Bar: record  {
	a: count;
	b: count;
};

type Foo: record  {
	a: count &default=42;
	b: set[string] &default=set();
	c: table[string] of count &default=table();
	d: set[string] &default=set("A", "B");
	e: table[string] of count &default=table(["a"] = 1, ["b"] = 2);
	f: Bar &default=[$a=1, $b=2];
	g: vector of count &default=vector();
	h: vector of count &default=vector(1,2,3);
};

event bro_init() &priority = 42
	{
	local f: Foo;
	print f$a;
	print f$b;
	print f$c;
	print f$d;
	print f$e;
	print f$f;
	print f$g;
	print f$h;
	}



