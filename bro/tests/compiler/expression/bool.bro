#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local s: table[count] of string;

	print T && T;
	print T && F;
	print F && T;
	print F && F;

	print "";
	
	print T || T;
	print T || F;
	print F || T;
	print F || F;

	print "";
	
	# Short circuit.
        print F && (s[10] == "X");
        print T || (s[10] == "X");
	}
