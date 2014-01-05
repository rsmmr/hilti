#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T Hilti::save_hilti=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local f: function(s: string);

	f = function(s: string) 
		{
		print "anon-func", s;
		};

	f("arg");
	}





