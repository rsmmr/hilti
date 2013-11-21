#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

global x: count;
global y = 2;
global z = [$a=2];

const a = 5;
const b = 7 &redef;
redef b = 8;

event bro_init() &priority = 42
	{
	print x;
	print y;
	print z;

	print a;
	print b;
	}
