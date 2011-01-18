
Feature Requests
================

* Some sort of looping construct.

This is a really low priority for now. There are already several ways of
effectively doing parsing loops. Unit recursion, lists (along with foreach
when parsing lists), and function recursion.

* List index operator with auto typecasting.

According to Robin, vectors are on the way which have efficient indexing.

* Fully print out bitfields when debugging with pac-driver.

For a bitfield like this::

	status        : bitfield(uint32) {
	        Severity : 0..1 &convert=StatusSeverity;
	        Customer : 2;
	        N        : 3;
	        Facility : 4..15;
	        Code     : 16..31;
	};

The output currently looks like this::

	[binpac]       status = '0'

It would be more helpful if it looked something like this::

	[binpac]       status
	[binpac]          Severity = 'Success'
	[binpac]          Customer = '0'
	[binpac]          N = '0'
	[binpac]          Facility = '0'
	[binpac]          Code = '0'

* Bit Shifting and logical operators for uints. Needed for timestamp parsing
  in SMB2.

Test found at <write test>

