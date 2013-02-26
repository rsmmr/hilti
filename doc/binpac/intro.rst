
.. .cd _examples

.. _pac2_intro:

Introduction
============

This introduction gives a short overview of how to write and use
BinPAC++ parsers. For a complete list of features available see
:ref:`pac2_reference`.

.. _simple:

Here's a simple "Hello, World!" in BinPAC++::

    module Test;

    print "Hello, world!"

Assuming that's stored in ``hello.pac2``, we can compile and run it
with :ref:`pac2_pac-driver` and then run it like this::

    > pac-driver hello.pac2
    Hello, World!

``pac-driver`` compiles the source code into native code on the fly,
and then runs it directly. Alternatively, :ref:`hilti_hilti-build`  produces a
stand-alone binary for subsequent execution::

    > hilti-build -o a.out tools/pac-driver/pac-driver.c hello.pac2
    > ./a.out
    Hello, World!

Note the inclusion of ``pac-driver.c`` here: BinPAC++ generated code
cannot run on its own but needs a *driver program* that provides a
``main`` function, and normally (though not in this trivial example)
also the input data for the generated parsers. ``pac-driver.cc`` is a
generic version of such a driver that can be used for testing and
debugging parsers from the command line, without the need for any
further host application. If you run the generated binary ``a.out``
with ``--help``, you'll see all the options that ``pac-driver.cc``
provides. 

.. note::

    The ``pac-driver`` tool is actually also compiled from the same
    ``pac-driver.cc`` code, with just a few smaller tweaks to turn the
    generic driver into JIT mode.

Note that the above hello-world program does not actually contain any
parser specification; just some global code executed at initialization
time. We'll see below how to write actual parsers.

.. _pac2_simple:

A Simple Parser
---------------

A BinPAC++ parser specification describes the layout of a protocol
data unit (PDU), along with semantic actions to perform when
individual pieces are parsed. Here's a simple example for parsing an
HTTP-style request line, such as ``GET /index.html HTTP/1.0``:

.. literalinclude:: examples/request.pac2

In this example, you can see a number of things:

    * A specification must always start with a ``module`` statement
      defining a namespace. 

    * The layout of a self-contained data unit is defined by creating
      a :pac2:type:`unit` type, listing its individual elements in the
      order they are to be parsed. In the example, there are two such
      units defined, ``RequestLine`` and ``Version``.

    * Each field inside a unit has a type and an optional name. The
      type defines how that field will be parsed from the raw input
      stream. In the example, all fields have a regular expression as
      their type, which means that the generated parser will match
      these expressions against the input stream in the order as the
      fields are layed out. Note how the regular expressions can
      either be given directly as a field's type (as in ``Version``),
      or indirectly via globally defined :ref:`Constants
      <pac2_global_constants>` (as in ``RequestLine``). Also note that
      if a field has a regular expression as it's type, the parsed
      value will later have a type of :pac2:type:`bytes`.

      If a field has a name, it can later be refereced for getting to
      its content. Consequently, all fields with semantic meanings
      have names in the example, while those which are unlikely to be
      relevant later don't (e.g., the whitespaces).

    * A unit field can have another unit as its type; here that's the
      case for the ``version`` field in ``RequestLine``. The meaning
      is straight-forward: when parsing the outer unit reaches that
      field, it first fully parses the sub-field accordings to that
      one's layout specification before it continues in the outer
      unit. 

    * We can specify code to be executed when a unit has been
      completely parsed by defining an `on %done` :ref:`hook
      <pac2_hooks>` The statements in the hook can refer to the
      current unit instance by using the implicitly defined ``self``
      identifier; and they can access the parsed fields by using a
      standard attribute notation (as used by other languages, such as
      Python or C++). As the access to ``version`` shows, this also
      works for getting to the fields of sub-units. In the example, we
      tell the generated parser to output three of the parsed fields
      when finished.

    * The ``export`` keyword declares the parser generated for a unit
      to be accessible from an external host application. Only
      exported units can later be the starting point for feeding in
      input, all other units can't be directly used for parsing (only
      indirectly as sub-units of an exported unit). 

Now let's see how we turn this into an actual parser. If we save the
above specification into the file ``request.pac2``, we can use
``pac-driver`` to execute it::

    > echo "GET /index.html HTTP/1.0" | pac-driver request.pac2
    GET /index.html 1.0

As you see, the parsing succeeds and the ``print`` statement wrote out
the three fields one would expect. If we pass something in that's 
malformed, the parser will complain::

    > echo "GET HTTP/1.0" | pac-driver request.pac2
    hilti: uncaught exception, ParseError with argument look-ahead symbol(s) [[ \t]+] not found


Exploring More
--------------

* The BinPAC++ :ref:`pac2_reference` is slowly growing. Eventually, it
  will document all available features. 

* There are two (preliminary) parsers for HTTP and DNS in
  ``libbinpac/protocols``.

* Look at the BinPAC++ source files (``*.pac2``) in the
  ``tests/binpac/*`` subdirectories to see how BinPAC++ grammars look
  like. In particular, ``test/unit/*.pac2`` show the various features
  available for expressing grammars. 

