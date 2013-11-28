#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
# 

event bro_init() &priority = 42
	{
	local s1 = set("A", "B", "C");
	local s2 = set(1,2,3);

	for ( i in s1 )
		for ( j in s2 )
			{
			if ( j == 2 )
				next;
			
			print i, j;
			}
	}
