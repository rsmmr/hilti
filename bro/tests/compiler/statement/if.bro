#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
# 

event bro_init() &priority = 42
	{
	if ( T )
		print "yes";
	else
		print "no";
	
	if ( F )
		print "no";
	else
		print "yes";

	if ( T )
		print "yes";
	
	if ( F )
		print "no";
	}
