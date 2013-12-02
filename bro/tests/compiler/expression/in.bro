#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local t1 = table([1] = "A", [2] = "B");

	print 1 in t1;
	print 10 in t1;
            
	local t2 = table([1,2] = "AA", [2, 3] = "BC");

	print [1,2] in t2;
	print [4,5] in t2;

	print "";

	##
	
	local s1 = set(1,2,3);
	print 1 in s1;
	print 10 in s1;

	local s2: set[count,count];
	add s2[1,2];
	add s2[2,3];
	print [1,2] in s2;
	print [10,20] in s2;

	print "";
	
	###

	print "456" in "1234567890";
	print "XXX" in "1234567890";
	
	print "";
	
	###
        
	print 10.0.1.1 in 10.0.1.0/24;
	print 10.0.1.1 in 10.0.2.0/24;
	}
