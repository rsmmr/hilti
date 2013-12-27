#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

module GLOBAL;

export {
        global id_string: function(id: string);
}

function id_string(id: string)
        {
	print "1", id;
        }

module HTTP;

export 
	{
	global x: function(c: count);
	}

function x(c: count)
	{
	id_string("X");
	}

module Foo;

event bro_init()
	{
	HTTP::x(12);
	}

