#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

function foo(n: count): string
	{
	return fmt("y-%d", n);
	}

global TTX: table[count] of string &default=function(n: count): string { return fmt("x-%d", n); };
global TTX2: table[count] of string &default=foo;
event bro_init() &priority = 42
	{
	TTX[21] = "xyz";

	print 21 in TTX;
	print TTX[21];
	
	print 42 in TTX;
	print TTX[42];
	
	print 42 in TTX2;
	print TTX2[42];
	}



