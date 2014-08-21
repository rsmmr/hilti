
module Bro;

export
	{
	global register_protocol_analyzer: function(a: Analyzer::Tag);
	}

# This doesn't work, as we can't pass the tag into the inner function.
#
# function register_protocol_analyzer(a: Analyzer::Tag)
# 	{
# 	Files::register_protocol(a, [$get_file_handle =
# 				     function(c: connection, is_orig: bool) : string
# 				     	{
# 					return cat(a, c$id, is_orig);
# 					}
# 				     ]
# 				 );
# 	}

global registered_analyzers: set[Analyzer::Tag];

function register_protocol_analyzer(a: Analyzer::Tag)
	{
	add registered_analyzers[a];
	}

event get_file_handle(tag: Analyzer::Tag, c: connection, is_orig: bool) &priority=-5
	{
	if ( tag !in registered_analyzers )
		return;

	set_file_handle(cat(c$id, tag, is_orig));
	}

