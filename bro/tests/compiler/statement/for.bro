#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
# 

event bro_init() &priority = 42
	{
	local s1 = set(1,2,3,4);
	print s1;

	for ( i in s1 )
		print i;
	print "";

	#
	
	local s2 = set([1, "A"], [2, "B"], [3, "B"]);
	print s2;

	for ( [i,s] in s2 )
		print i, "|", s;
	print "";

	# 

	local s3 = table([1] = "A", [2] = "B", [3] = "B");
	print s3;

	for ( i in s3 )
		print i;
	print "";

	# 
        
	local s4 = vector(1,2,3,4);
	print s4;

	for ( i in s4 )
		print i;
	print "";

	#
	for ( i in s1 )
		for ( j in s3 )
			print i, j;
	
	}
