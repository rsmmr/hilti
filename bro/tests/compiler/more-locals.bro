#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

module Foo;

export 
	{
	type R: enum { A, B, C, D };
	}

module GLOBAL;

global x: set[Foo::R] = { Foo::A, Foo::B, Foo::C };

event bro_init() &priority = 42
	{
	for ( i in x )
		print i;
	}

function foo(a: int)
	{
	print a;
	}

event bro_init() &priority = 42
	{
	local bar1: count;
	local bar2 = 1;
	print bar1;
	print bar2;

	foo(42);
	}


