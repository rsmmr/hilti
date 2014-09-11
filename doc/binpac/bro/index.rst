
.. _pac2_bro-plugin:

=============
Bro Interface
=============

.. contents::

BinPAC++ comes with a Bro plugin that adds support for ``*.pac2``
grammars to Bro. The following overview walks through installation and
usage. Keep in mind that everything is in development and not
necessarily stable right now.

Installation
============

You currently need the current development version of Bro, as that
adds support for dynamically loaded plugins. To get it, use git::

    > git clone --recursive git://git.bro.org/bro

Now you need to build Bro with the same C++ compiler (and C++ standard
library) that you also use for compiling HILTI/BinPAC++. If
that's, say, ``/opt/llvm/bin/clang++``, do:: 

    > CXX="/opt/llvm/bin/clang++ --stdlib=libc++" ./configure && make

Then compile HILTI/BinPAC++, pointing it to your Bro clone::

   > make BRO_DIST=/path/to/where/you/cloned/bro/into

Watch the output, it should indicate that it has found Bro.

If things went well, the Bro tests should now succeed::

   > cd bro/tests && make
   [...]
   all XXX tests successful

Cool.

.. note::

    The Bro plugin has no ``make install`` for now, one needs to run
    it right out of the build directory. See below for how to do that.

Usage
=====

Writing a BinPAC++ analyzer for Bro consists of two steps: writing the
analyzer as standard ``*.pac2`` code; and defining the analyzer's
interface to Bro, including the events it will generate, in a separate
``*.evt`` file. We look at the two separately in the following, using
a simple analyzer for parsing SSH banners as our running example. We
then show to run Bro so that it loads the BinPAC++ plugin along with
any analyzers.

BinPAC++ Grammar
----------------

Here's a simple grammar ``ssh.pac2`` to parse an SSH banner (e.g.,
``SSH-2.0-OpenSSH_6.1``):

.. literalinclude:: /../bro/pac2/ssh.pac2

This is indeed just a standard BinPAC++ grammar, just as it can be
used with :ref:`pac2_pac-driver`. It could include further hooks and
also global code; the latter will execute once at Bro initialization
time. If you add ``print`` statements, they will generate the
corresponding output as Bro processes input, which can be helpful for
debugging.

Protocol Analyzer Interface
---------------------------

Next, we define a file ``ssh.evt`` that refers to the grammar to
define a new Bro analyzer. An ``*.evt`` file has three parts: (1) an
``grammar`` specification telling Bro which ``*.pac`` file to use; (2)
an ``analyzer`` definition describing where to hook it into Bro's
traffic processing; and (3) a series of event definitions specifying
how to turn what's parsed into the Bro events. Here's an ``ssh.evt``
to go with our SSH grammar:

.. literalinclude:: /../bro/pac2/ssh.evt

The ``grammar`` line specifies a ``*.pac2`` file to load. The path is
relative to the location of the ``*.evt`` file; if it's not found
there, the plugin further searches for grammars inside the source tree
at ``hilti2/bro/pac2/``, and in any directories specified through the
environment variable ``BRO_PAC2_PATH`` (using the standard syntax of
colon-separated paths). The ``grammar`` line is optional, one can
leave it out if the ``*.pac2`` grammars get loaded otherwise (e.g.,
via the command line; see below). One can also specify multiple
``grammar`` entries.

.. note::

    Technically, the Bro plugin searches the ``*.pac2`` files inside
    the *build* directory at ``<build>/pac2`` because that will later
    correspond to the plugin's top-level installation directory
    (installing a plugin essentially means copying the build directory
    somewhere else). The current build system setup however links that
    back to the source directory ``hilti2/bro/pac2/`` so that one can
    edit files there and they will be pulled in directly.

The ``analyzer`` block starts with giving the new analyzer a name
(``pac2::SSH``). The namespace isn't mandatory, but we use it to
differentiate BinPAC++ analyzers from Bro's standard analyzers. The
name corresponds to how we'll refer to the new analyzer in Bro. For
example, just like there's an ``analyzer::ANALYZER_SSH`` enum for the
standard SSH analyzer, there'll now be an
``analyzer::ANALYZER_PAC2_SSH``. (As you can see there's a bit of name
normalization going on, use ``bro -NN`` to see the final name.).
``over TCP`` declares this to be an TCP application-layer analyzer
(``over UDP`` is the one other supported alternative right now).

Next comes set of of properties for the analyzer, separated by commas
(order is not important):

    - ``parse with <unit>`` links the new analyzer with the BinPAC++
      grammar: ``<unit>`` is the fully-qualified name of the unit we
      want to use as the top-level entry point for parsing. When
      specified the way we do here, the unit will be used for both
      originator- and responder-side traffic. One can use different
      units for each side by specifying ``parse originator with
      <orig-unit>`` and ``parse responder with <resp-unit>``,
      respectively.

    - ``port <port>`` defines the well-known port for the analyzer;
      this translates into registering the port with Bro's analyzer
      framework at runtime. This is optional; as usual analyzers can
      also be activated via DPD signatures or even manually from
      script-land. One can give multiple ``port`` specifications (but
      separately).

    - ``replaces <analyzer>`` tells Bro that when this analyzer is
      activateed, it's replacing one of the built-in analyzers, given
      by its name (see again ``bro -NN`` for the names of all built-in
      analyzers). This has two effects: First, the built-in one will
      be completely disabled at startup, and second in script-land Bro
      will use the name of the original analyzer in place of this one
      (e.g., so that in ``conn.log`` the service will show up as
      ``SSH``, not ``PAC2_SSH``).

Events are defined by lines of the form ``<hook> -> event
<name>(<parameters>)``. Let's break it down:

    - ``<hook>`` defines when an event should be triggered, and it
      works just like externally defined BinPAC++ :ref:`hooks
      <pac2_hooks>`, i.e., you give the fully-qualified path to the
      grammar element that is to trigger the event during parsing. In
      the example above, ``on SSH::Banner`` will trigger an event
      whenever an instance of the ``SSH::Banner`` unit has been fully
      parsed. One can also refer to individual unit fields and
      variables (e.g., ``on SSH::Banner::version``). The unit-wide
      unit hooks work as well (e.g., ``on SSH::Banner::%init``).

    - ``<name>`` corresponds directly to the name of the event as
      visible in Bro. As you can see, one can (and should) use
      namespaces.

    - The ``<parameters>`` define the event's parameters, specifying
      both their values and (implicitly) their types at the same time.
      Each parameter is a complete BinPAC++ expression (except for
      some "magic" ones starting with ``$``, see below). The type of
      each BinPAC++ expression determines the Bro type of the
      corresponding event parameter, with an internal mapping defining
      how each BinPAC++ type translates into a Bro type (e.g., a
      ``bytes`` objects translates into a Bro ``string``; see
      :ref:`pac2_bro-type-mapping` for the details). At runtime, when
      an event is raised, the expression is evaluated to determine the
      value passed to event's handlers. That evaluation takes place in
      a hook context corresponding to what ``<hook>`` triggers on, and
      it has access to the same scope as a manually written hook would
      have. For example, with the ``on SSH::Banner`` event, ``self``
      refers to a ``SSH::Banner`` unit instance and ``self.version``
      to its ``version`` item. If, for example, you wanted an
      upper-case version of the software identification, you could use
      ``self.software.upper()``.

      In addition to expression parameters, there are three "magic"
      parameters that provide access to internal Bro state:

        - ``$conn`` references the current connection that's being
          parsed, and it translates into a ``connection`` record
          parameter for the corresponding Bro event.

        - ``$is_orig`` turns into a boolean value indicating whether
          BinPAC++ is parsing the originator or responder side of the
          connection.

        - ``$file`` references the current file in case of defining a
          file analyzer rather than a protocol analyzer (see below).

      Note that the magic parameters aren't expressions; you can't
      further manipulate them.

Using a BinPAC++ Analyzer
-------------------------

With both the grammar and the analyzer/event definition in place, we
can now write an event handler:

.. literalinclude:: /../bro/tests/pac2/ssh-banner.bro
    :lines: 6-9

Note how the ``ssh::banner`` definition from ``ssh.evt`` maps into the
event's parameter signature.

Before we can use it, we need to tell Bro where to find the BinPAC++
plugin. For that, we set the environment variable ``BRO_PLUGIN_PATH`` to
the plugin's build directory::

    export BRO_PLUGIN_PATH=/path/to/binpacpp/hilti2/build/bro

.. note::

    If you get the path to the plugin wrong, or forget to set it at
    all, you'll get some misleading error messages below as Bro will
    try to parse ``ssh.evt`` file as a Bro script.

Now we can run Bro with the new analyzer by giving it the ``ssh.evt``
on the command line. If not an absolute path, Bro will search for the
the file first in the current directory first, and then---just as
described above for ``*.pac2`` files---in ``hilti2/bro/pac2/`` and any
directories specified by ``BRO_PAC2_PATH``.

Let's first just check that Bro indeeds loads the analyzer correctly.
``bro -NN`` will tell us::

    # bro -NN ssh.evt
    [...]
    Plugin: Bro::Hilti - Dynamically compiled HILTI/BinPAC++ functionality (dynamic, version 1)
        [Analyzer] pac2_SSH (ANALYZER_PAC2_SSH, enabled)
        [Event] ssh::banner
        [...]
    [...]

.. todo::

    The ``-NN`` output currently does not include all the information
    shown above. Need to fix.

We see that Bro has found the BinPac++ plugin. The plugin indeed
provides our analyzer ``pac2_SSH``, which generates one event
``ssh_banner``.

Let's now try processing a trace containing a single SSH connection,
making sure to give Bro our event handler ``ssh-banner.bro`` as well::

    # bro -r ssh-single-conn.trace ssh.evt ssh-banner.bro
    SSH banner, [orig_h=192.150.186.169, orig_p=49244/tcp, resp_h=131.159.14.23, resp_p=22/tcp], F, 1.99, OpenSSH_3.9p1
    SSH banner, [orig_h=192.150.186.169, orig_p=49244/tcp, resp_h=131.159.14.23, resp_p=22/tcp], T, 2.0, OpenSSH_3.8.1p1

File Analyzers
--------------

Writing a BinPAC++ file analyzer works pretty similar to a protocol
analyzer. The main difference is indeed just that we need to tell Bro
to use our BinPAC++ unit for parsing files rather than protocols. We
do that via the ``*.evt`` file by defining a ``file analyzer`` section
instead of a ``protocol analyzer``. Here's an example for a GZIP
decoder:

.. literalinclude:: /../bro/pac2/gzip.evt

The ``parse with`` works the same as with a protocol: it specifies the
top-level BinPAC++ unit to use. Instead of giving a well-known port to
activate the analyzer automatically (as we do with protocols), we can
associate a file analyzer with a MIME type. Now whenever some part of
Bro finds data of that type, it will deploy our BinPAC++ analyzer to
take it apart.

Here's the main part of the corresponding grammar file ``gzip.pac2``
(we skip the definition of the ``OS`` enum for brevity):

.. literalinclude:: /../bro/pac2/gzip.pac2
    :lines: 6-32

For testing let's also provide an event handler in ``gzip-header.bro``:

.. literalinclude:: /../bro/tests/pac2/gzip-header.bro
    :lines: 6-9

With that all in place, we can run Bro like this on a trace with a
GZIP file transferred via HTTP::

    # bro -r gzip-single-request.trace gzip.evt gzip-header.bro
    7aNwGytV9k4, 8, 0, 1380302739.000000, 0, gzip::UNIX

HILTI/BinPAC++ Options
----------------------

Bro's BinPAC++ plugin comes with a number of options defined in
``hilti2/bro/scripts/bro/hilti/base/main.bro``. These include:

``debug: bool`` (default: false)
    If true, compiles all BinPAC++/HILTI code in debug mode, meaning
    that :ref:`pac2_debugging` will be available (including the
    ``HILTI_DEBUG`` environment variable).

``dump_debug: bool`` (default: false)
    Dumps debug information about the compiled analyzers to the
    console. Can be helpful if Bro isn't integrating a BinPAC++
    analyzer as expected.

``optimize: bool`` (default: true)
    Activates code optimization. Should be on normally, but makes
    debugging in the debugger easier if off.

``use_cache: bool`` (default: true)

    .. todo::

        This is currently broken and disabled.

    Enables caching of compiled BinPAC++/HILTI code. If on, you will
    notice that the first time you start Bro with a new (or modified)
    analyzer, it takes longer than on subsequent invocations. That's
    because the first one needs to do all the leg-work of going from
    BinPAC++ to native code. It then however caches the resulting code
    on disk and reuses it next time directly.

    Normally you want this on, however currently the cache does not
    always pick up on changes made to ``*.pac2`` or ``*.evt`` files
    (or Bro itself, or the BinPAC++ plugin). If you notice that a
    change doesn't seem to take effect, try removing the cache
    directory (``.cache``) or simply disable it altogether.

See the script itself for the complete list of all options.

.. _pac2_bro-type-mapping:

Type Mapping
------------

The following table summarizes how BinPAC++ types get mapped into Bro
types. Types not listed are not yet implemented.

=============  ==============
BinPAC++       Bro
=============  ==============
``addr``       ``addr``
``bool``       ``bool``
``bytes``      ``string``
``double``     ``double``
``enum``       ``enum`` [1]
``int<*>``     ``int``
``string``     ``string``
``time``       ``time``
``uint<*>``    ``count``
``tuple<*>``   ``record`` [2]
``list<*>``    ``vector``
``set<*>``     ``set``
``vector<*>``  ``vector``
=============  ==============

[1] A corresponding Bro ``enum`` type is created automatically.

[2] The record fields will by default be named ``f<n>`` with *n* being
the zero-baed element index. However, if the tuple gets passed into a
Bro event, the initially created record will be coered into the type
the event expects and hence then use the corresponding field names
instead.

