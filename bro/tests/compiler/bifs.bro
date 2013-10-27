#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

event bro_init() &priority = 42
	{
	print Hilti::is_compiled();                   # HILTI-bif overriding legacy bif.
	print mangle_string("Foo");                   # HILTI-only bif in global namespace.
	print HiltiTesting::mangle_string("Foo");     # HILTI-only bif in module namespace.
	print to_lower("FoObAr");                     # Legacy global bif.
        print "";
	}

# TODO: Once we can call Bro functions, write functions that do the same code
# from inside other modules. The code below puts everything into the GLOBAL
# namespace as that's where bro_init() is defined in.

# module XXX;
# 
# event bro_init() &priority = 42
# 	{
# 	print Hilti::is_compiled();                   # HILTI-bif overriding legacy bif.
# 	print mangle_string("Foo");                   # HILTI-only bif in global namespace.
# 	print HiltiTesting::mangle_string("Foo");     # HILTI-only bif in module namespace.
# 	print to_lower("FoObAr");                     # Legacy global bif.
#         print "";
# 	}
# 
# module Hilti;
# 
# event bro_init() &priority = 42
# 	{
# 	print Hilti::is_compiled();                   # HILTI-bif overriding legacy bif.
# 	print mangle_string("Foo");                   # HILTI-only bif in global namespace.
# 	print HiltiTesting::mangle_string("Foo");     # HILTI-only bif in module namespace.
# 	print to_lower("FoObAr");                     # Legacy global bif.
#         print "";
# 	}
# 
