
# A set of bifs for testing. These are all implemented inside the HILTI runtime.

global mangle_string: function(s: string ): string;

module HiltiTesting;

export {
	global mangle_string: function(s: string ): string;
}

