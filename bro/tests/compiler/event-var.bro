#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T Hilti::save_hilti=T | sed 's/0x.*/0xXXX/g' >output
# @TEST-EXEC: btest-diff output
#
#
event foo1(s: string)
	{
	print "foo1a", s;
	}

event foo1(s: string)
	{
	print "foo1a", s;
	}

event foo1(s: string)
	{
	print "foo1b", s;
	}

event bro_init() &priority = 42
	{
	local e: event(s: string);
	e = foo1;
	print e;

	# Note, we can't actually raise an event indirectly. The scripting language
	# doesn't support that, only the core can do it. (Though it shouldn't
        # be hard to extend the compiler to support it, the main missing piece
        # is an implementation of ModuleBuilder::HiltiCallEventLegacy).
        #event e("arg");
	}



