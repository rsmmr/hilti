#
# @TEST-EXEC: bro -b %INPUT Hilti::compile_scripts=T >output
# @TEST-EXEC: btest-diff output
#

@load base/utils/queue.bro

type R: record
	{
	i: count &default = 0;
	};

event bro_init() &priority = 42
	{
	local q = Queue::init();
	local r1: R;
	local r2: R;

	r1$i = 10;
	print r1;

	Queue::put(q, r1);

	r1$i = 20;

	r2 = Queue::peek(q);
	print r2;
	}


