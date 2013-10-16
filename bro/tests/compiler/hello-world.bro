#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	print "Hello HILTI World!";
	print Hilti::is_compiled();
	print to_lower("FoObAr");
	}




