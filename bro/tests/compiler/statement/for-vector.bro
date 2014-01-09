#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
# 

event bro_init() &priority = 42
	{
	local s1 = vector(1,2,3,4);
	print s1;

	for ( i in s1 )
		print i, s1[i];
	print "";

	###

	local s2 = vector("A","B","C");
	print s2;

	for ( i in s2 )
		print i, s2[i];
	print "";
	
	###

	for ( i in s2 ) 
		{
		if ( i == 1 )
			break;
		
		print i, s2[i];
		}
	
	print "";

	###

	for ( i in s2 ) 
		{
		if ( i == 1 )
			next;
		
		print i, s2[i];
		}
	
	print "";
	
	}

