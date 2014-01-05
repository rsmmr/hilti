#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	local s: table[count] of string;

	print T ? "YES" : "NO";
	print !T ? "YES" : "NO";

	print 4 in s ? s[4] : "-";
	}
