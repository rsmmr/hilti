
.. _pac2_debugging:

Debugging Support
-----------------

It can be tough to track down the specifics of what a parser is doing
(or not doing) because often there's no directly observable effect.
However, Spicy comes with support that helps with debugging.

The simplest way to learn more about what's going on is to add a hooks
with ``print`` statements to the grammar. That's rather disruptive
though, and hence there are also special :ref:`%debug <pac2_hooks>`
unit hooks which only get compiled into the resulting code if
:ref:`pac2_pac-driver` is run with both the ``-d`` and ``-g`` options
(the former leads to generating code with debugging support, and the
latter enables the hooks)::

    export type test = unit {

       a: bytes &length=4 %debug {
            print self.a;
            }

       b: bytes &length=6;

       on b %debug { print self.b; }
   };


The second form of debugging support uses HILTI's *debugging streams*
to instrument the generated parsers for logging activity as they
proceed parsing input. For this to be compiled in, ``pac-driver`` must
be run with ``-d``. The output can then be enabled by setting the
environment variable ``HILTI_DEBUG`` to the name of an output stream
to select the desired information; see below for the list. HILTI
records the output in ``hlt-debug.log``. Here's an example using the
stream ``binpac``, which logs unit fields as they are parsed::

    > echo "GET /index.html HTTP/1.0" | HILTI_DEBUG=binpac pac-driver -d request.pac2
    GET /index.html 1.0
    > cat hlt-debug.log
    00000001 [binpac/main-thread]           RequestLine
    00000002 [binpac/main-thread]             method = GET
    00000003 [binpac/main-thread]             __anon1 =
    00000004 [binpac/main-thread]             uri = /index.html
    00000005 [binpac/main-thread]             __anon2 =
    00000006 [binpac/main-thread]             version
    00000007 [binpac/main-thread]               __anon4 = HTTP/
    00000008 [binpac/main-thread]               number = 1.0
    00000009 [binpac/main-thread]             __anon3 = \x0a

The following debugging streams are currently available:

``binpac``
    Logs unit fields and variables as they are parsed (or more
    generally, as they get values assigned). This is often the most
    helpful output as this shows rather concisely what the parser is
    doing, and in particular how far it gets in cases where it doesn't
    parse something correctly.

``binpac-verbose``
    Logs many internals about the parsing process, including the
    grammar rules currently being parsed; the current input position;
    lexer tokens; and look-ahead symbols which might be pending; and
    more.

    This stream is primarily intended for debugging the Spicy
    compiler itself.

``hilti-trace``
    This is a HILTI-defined debugging level that records every HILTI
    instruction being executed, along with arguments and return
    values.

    This stream is primarily intended for debugging the Spicy
    compiler itself.

``hilti-flow``
    This is a HILTI-defined debugging level recording all function
    calls and returns.

    This stream is primarily intended for debugging the Spicy
    compiler itself, though it may also be helpful to understand the
    internal control flow when writing a grammar.
    
Note that multiple streams can be enabled by separating them with
colons. Furthermore, when using :ref:`hilti_hilti-build` with its
``-d`` options, the ``HILTI_DEBUG`` works in the same way with the
generated executable.

Also note that generating code with debugging instrumentation (``-d``)
can be quite a bit slower.

