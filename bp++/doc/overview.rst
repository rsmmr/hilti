
.. .cd _examples

.. _overview:

Introduction
============

This introduction gives a short overview of how to write and use
BinPAC++ parsers. For a complete list of features available see
:ref:`reference`.

.. _simple:

A Simple Parser
---------------

A |binpac| parser specification describes the layout of a PDU, along
with semantical actions to perform when individual pieces are
parsed. Here's a simple example for parsing an HTTP-style request
line, such as ``GET /index.html HTTP/1.0``:

.. 

.. literalinclude:: _examples/request.pac2

In this example, you can see a number of things:

    * A specification must always start with a ``module`` statement
      defining a namespace. 

    * The layout of a self-contained data unit is defined by
      creating a ``unit`` type, listing its individual elements in
      the order they are to be parsed. In the example, there are two
      such units defined, ``RequestLine`` and ``Version``.

    * Each field inside a unit has a type and an optional name. The
      type defines how that field will be parsed from the raw input
      stream. In the example, all fields have a regular expression
      as their type, which means that the generated parser will
      match these expressions against the input stream in the order
      as the fields are layed out. Note how the regular expressions
      can either be given directly as a fields' type (as in
      ``Version``), or indirectly via globally defined constants (as
      in ``RequestLine``). Also note that if a field has a regular
      expression as it's type, the parsed value will later have a
      type of ``bytes``.

      If a field has a name, it can later be refereded for getting
      to its content. Consequently, all fields with semantic
      meanings have names in the example, while those which are
      unlikely to be relevant later don't (e.g., the whitespaces).

    * A unit field can have another unit as its type; here that's
      the case for the ``version`` field in ``RequestLine``. The
      meaning is the obvious one: when parsing the outer unit
      reaches that field, it first fully parses the sub-field
      accordings to that one's layout specification before it
      continues in the outer unit. 

    * We can specify code to be executed when a unit has been
      completely parsed with the `on %done` handler. The statements
      in the handler can refer to the current unit instance bu using
      the implicitly defined ``self`` identifier; and they can
      access the parsed fields by using a standard attribute
      notation (as used by other language, such as Python or C++);
      as access to ``version`` shows, this also works for getting to
      the fields of sub-units. In the example, we tell the generated
      parser to output three of the parsed fields when finished.

    * The ``export`` keyword declares the parser generated for a
      unit to be accessible from an external host application. Only
      exported units can be the starting point for feeding in input,
      all other units can't be directly used for parsing (only
      indirectly as sub-units of an exported unit). 

Now let's see how we turn this into an actual parser. If we save the
above specification into the file ``request.pac2``, we can compile
it into a stand-alone parser by combining it with the generic |pd|::

    > make-pac-driver -o request -d request.pac2

.. note:: ``make-pac-driver`` is a script found in
   ``bp++/tools/pac-driver``. It's a simple wrapper around |hb|
   that compiles the given ``*.pac2`` specification and links it
   with |pd|, the generic stand-alone driver (also setting a few
   additional search paths as required). The wrapper passes all
   arguments directly on to |hb|. 

The option ``-o`` tells |hb| where to store the final binary.

We can now run the parser by passing it some data on standard
input::

    > echo "GET /index.html HTTP/1.0" | ./request
    GET /index.html 1.0

As you see, the parsing succeeded and the ``print`` statement wrote
out the three fields as expected. If we pass something in that's not
malformed, the parser will complain::

    > echo "GET HTTP/1.0" | ./request
    hilti: uncaught exception, ParseError with argument look-ahead symbol(s) [[ \t]+] not found

Defining Units
--------------

Fields
~~~~~~

TODO.

Variables
~~~~~~~~~

TODO.

.. _hooks:

Hooks
~~~~~

The ``on %done { ... }`` code shown in :ref:`simple` is an example
of a *hook*. A hook is code that is will be executed during parsing
at certain points of time. There are two types of hooks:

Field Hooks
    Field hooks are associated with a specific field and executed
    whenver that field has been fully parsed. For example, the
    following code snippet adds a hook to the ``uri`` field in the
    earlier example, in this case simply printing the field's
    content::

        ...
        uri: Token {
            print self.uri;
            }
        ...

   Note that the ``self`` identifier still refers to the unit
   instances (and not to the field.).

   Often, a field hook will store information in other variables,
   like in this toy example::

        ...

        uri : Token {
            if ( self.uri.startswith("http://") )
                self.proxy = True;
            }

        var proxy : bool;

        ...

Unit Hooks
    Unit hooks are not tied to a specific field but apply to the
    parsing process of the unit they are part of. They are always
    defined using the ``on <hook-name>`` syntac.  Currently, these
    unit hooks defined:

    ``%init``
        Executed when the parsing of a unit is about to start. All
        of the fields and variables will have their default values
        at this time (which may be unset and thus trigger and
        exception when being read). 

    ``%done``
        Executed when the parsing of a unit instance has completed.

    ``%debug``
        Executed when the parsing of a unit instance has completed
        *and* debugging support has been compiled in and is enabled
        at run-time. See :ref:`debugging` for more information about
        debugging. 

Item Hooks
    TODO: List items.

There are different locations to define a hook. As seen, field
hooks can be specified inline. Alternative, the ``on <name>`` syntax
works for them as well::

    type RequestLine = unit {
        ...
        uri:     Token;
        ...

        on uri {
            print self.uri;
        }
    };


Alternatively, all hooks can be defined outside of the actual type
definition as well, thereby *adding* code to an already defined type
externally::

    type RequestLine = unit {
        ...
        uri:     Token;
        ...
    };

    on RequestLine::uri {
        print self.uri;
        }

    on RequestLine::%done {
        ...
        }

Note that parsing will always execute *all* relevant hooks defined
anywhere in the input specification. It's perfectly fine to define a
field hook inline and later add more external hooks to the same
field; all of them will be executed (by default, in an undefined
order). This even works across units: if you ``import`` the
``Request`` module, into the another specification, you can add a
hook to it like this::

    on Request::RequestLine::uri {
        print self.uri;
        }

Using |pd|
----------

The |pd| is a generic host application acting as a stand-alone
driver for |binpac| parsers. Its purpose is to both enable
standalone development and testingm, and provide an example of
interfacing to the generated parsers.

As shown in :ref:`simple`, one compiles the |pdc| along with the parser(s),
most easily by using the ``make-pac-driver`` wrapper script. Run the generated
binary with the ``-h`` option to see a list of options, including the parsers
available (which correspond to the unit types exported in the input specification)::

    > ./request
    ./request [options]

      Options:

        -p <parser>[/<reply-parsers>]  Use given parser(s)

        -d            Enable pac-driver's debug output
        -B            Enable BinPAC++ debugging hooks
        -i  <n>       Feed input incrementally in chunks of size <n>
        -U            Enable bulk input mode

      Available parsers: 

        Request::RequestLine     (No description)

The ``-d`` option enables verbose logging of what |pd| is doing,
primarily for incremental and bulk modes; see below. The ``-B``
option enables  ``%debug`` hooks; see :ref:`debugging`.

Incremental Mode
~~~~~~~~~~~~~~~~

The *incremental* mode is primarily for testing the correct
functioning of the generated parsers: |binpac| analyzers are fully
incremental in the sense that you can feed them the input in
arbitrary pieces; a parser parses as much as it can and then yields
back to the host application until further input becomes available.
By default, |pd| first reads any data available on standard input
completely and then passes it into the parser in a *single* chunk of
bytes. In incremental mode, however, passes consecutive chunks of
the given size to the parser. 

Note that there should be no *difference* in output/results between
standard and incremental parsing of any chunk size. If there is,
that's a bug. Running with ``-i 1`` is a good stress test for
generated parsers. 


Bulk Mode
~~~~~~~~~

Normally, |pd| passes the input on standard input to a single
instance of the specified parser. In other words, the data is
assumed to contain a single parsing unit. In *bulk* mode, |pd|
instead mimics how a typical host application would use a |binpac|
parser by instantiating many *concurrent* parsers and feeds them
with interleaved chunks of incremental input.

For bulk mode, the input needs to be specifically prepared. There's
a `Bro <http://www.bro-ids.org>`_ script coming with |pd|, called
``pac-contents.bro``, that records the raw payload of UDP and TCP
packets in a form that's suitable as input for the pac-driver. The
generated data file, ``pac-contents.dat``, consists of individual
chunks of payload, just as Bro would pass them into one of its
protocol analyzers. For UDP, each chunk corresponds to one packet,
along with meta information identifying the flow. For TCP, each
chunk is a piece of the reassembled (i.e., in-order) payload stream,
likewise along with further connection information.

When started with ``-U``, |pd| expects such a ``pac-contents.dat``
file on standard input. It then instantiates one parser for each
(uni-directional) *flow* encountered, feeding it the corresponding
chunks successively (and interleaved with passing input to other
instances). Each direction of a connection is passed into a
*seperate* instance of the parser. One can define different parser
types for use with orginator- and responder-side flows,
respectively, by specifying two parsers to ``-p`` (seperated with a
slash). 

Here's an example of using bulk mode with the HTTP parser coming
with |binpac|::

    # Compile the HTTP parser.
    > make-pac-driver -o http libbinpac/protocols/http.pac2

    # Record some HTTP traffic with tcpdump.
    > tcpdump -i en0 -s 0 -w http.pcap port 80

    # Turn the trace into bulk input for pac-driver.
    > bro -r http.pcap pac-contents

    # Pass Bro's output into the parser.
    > cat pac-contents.dat | ./http -U -p HTTP::Requests/HTTP::Replies

Debugging
---------

It can be tough to track down the specifics of what a parser is
doing (or not doing) because often there's no directly observable
effect. However, there are tools for helping with debugging.

The simplest way to learn more about what's going on is to add
:ref:`hooks` to a grammar that contain ``print`` statements
displaying what one is interested in. As that's however rather
disruptive there are also special ``%debug`` hooks which only get
compiled into the resulting binary if |hb| is run with the ``-d``
option. When compiled in, ``%debug`` hooks are executed if debugging
is enabled at run-time via the :ref:`C API <c-api>`; which is
exactly what |pd|'s ``-B`` option is doing.

The second form of debugging support uses HILTI's *debugging
streams* to instrument the generated parsers for logging activity as
they proceed. For this to be compiled in, |hb| must likewise be run
with ``-d`` (or, for some, with the higher debugging level ``-D``).
Then at run-time the output can be enabled by setting the
environment variable ``HILTI_DEBUG`` to the name of a stream as
listed below. Here's an example using the stream ``binpac``, which
logs unit fields as they are parsed::

    > make-pac-driver -d -o request -d request.pac2
    > echo "GET /index.html HTTP/1.0" | HILTI_DEBUG=binpac ./request
    [binpac] RequestLine
    [binpac]    method = 'GET'
    [binpac]    uri = '/index.html'
    [binpac]    Version
    [binpac]       number = '1.0'
    GET /index.html 1.0

The following debugging streams are currently available:

``binpac``
    Logs unit fields as they are parsed (or more generally, as they
    are assigned values). This is often the most helpful output as
    this shows rather concisely what the parser is doing, and in
    particular how it gets in cases where it doesn't parse something
    correctly.

``binpac-verbose``
    Logs many internals about the parsing process, including the
    grammar rules currently being parsed; the current input
    position; lexer tokens; look-ahead symbols which might be
    pending; and more.

    ..note:: The ``binpac-verbose`` can be quite confusing right
    now, in particular without further knowledge about the parsing
    process. We should clean this up so that it becomes easier to
    undestand the output. 

``hilti-trace``
    This is a HILTI-defined debugging level recording every HILTI
    instruction being executed, along with arguments and return
    values. To enable this stream, one needs to increase the |hb|
    debugging level to ``-D``.

``hilti-flow``
    This is a HILTI-defined debugging level recording all function
    calls and returns. To enable this stream, one needs to increase
    the |hb| debugging level to ``-D``.

Note that multiple streams can be enabled by separating them with
colons. 
