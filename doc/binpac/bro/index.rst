
.. _bro-plugin:

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

You currently need a topic branch of Bro that adds support for
dynamically loaded plugins. To get it, use git::

    > git clone --recursive git://git.bro.org/bro
    > git checkout topic/robin/dynamic-plugins

Now you need to build Bro with the same C++ compiler that you use to
compile HILTI/BinPAC++ with. If that's, say,
``/opt/llvm/bin/clang++``, do:: 

    > CXX=/opt/llvm/bin/clang++ ./configure && make

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
then show to tell Bro where to find the BinPAC++ plugin and its
analyzers.

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

By default, for now Bro searches for grammars only inside the source
tree at ``hilti2/bro/pac2/``. It automatically pulls in all the
``*.pac2`` files it finds there. In addition, one can set the
environment variable ``BRO_PAC2_PATH`` to a colon-separated list of
further directories to search.

.. note::

    Technically, the Bro plugin searches the ``*.pac2`` files inside
    the *build* directory at ``<build>/pac2`` because that will later
    correspond to the plugin's top-level installation directory
    (installing a plugin essentially means copying the build directory
    somewhere else). The current build system setup however links that
    back to the source directory ``hilti2/bro/pac2/`` so that one can
    edit files there and they will be pulled in directly.

Analyzer Interface
------------------

Next, we define a file ``ssh.evt`` that refers to the grammar to
define a new Bro analyzer. Searches for ``*.evt`` files at the same
places as ``*.pac2`` grammars, and likewise pulls in all it finds
there.

An ``*.evt`` file has two parts, an analyzer definition describing
where to hook it into Bro's traffic processing a series of event
definitions specifying how to turn what's parsed into the Bro events:

.. literalinclude:: /../bro/pac2/ssh.evt


The ``analyzer`` block starts with giving the new analyzer a name
(``pac2::SSH``). The namespace isn't mandatory, but we use it
differentiate BinPAC++ analyzers from Bro's standard analyzer. The
name corresponds to how we'll refer to the new analyzer in Bro. For
example, just like there's an ``analyzer::ANALYZER_SSH`` enum for the
standard SSH analyzer, there'll now be an
``analyzer::ANALYZER_PAC2_SSH``. (As you can see there's a bit of name
normalization, use ``bro -NN`` so the final name.). ``over TCP``
declares this to be an TCP application-layer analyzer (nothing else is
supported yet).

Next comes set of of properties for the analyzer, separated by commas
(order is not important):

    - ``parse with <unit>`` links the new analyzer with the BinPAC++
      grammar: ``<unit>`` is the fully-qualified name of the unit we
      want to use as the top-level entry point for parsing. When
      specified as here, the unit will be used for both originator-
      and responder-side traffic. One case use different ones for each
      side by specifying ``parse originator with <orig-unit>`` and
      ``parse responder with <resp-unit>``, respectively.

    - ``port <port>`` defines the well-known port for the analyzer;
      this translates into registering the port with Bro's analyzer
      framework at runtime. This is optional, as usual analyzers can
      also be activated via DPD signatures or even manually from
      script-land. One can also give multiple ``port`` specifications
      (but separately).

    - ``replaces <analyzer>`` tells Bro that when this analyzer is
      activateed, it's replacing one of the built-in analyzers, giving
      by its name (again see ``bro -NN`` to get the names of all
      built-in analyzers). This has two effects: First, the built-in
      one will be completely disabled at startup, and second in
      script-land Bro will use the name of the original analyzer in
      place of this one (e.g., so that in ``conn.log`` the service
      will show up as ``SSH``, not ``PAC2_SSH``).

Events are defined by lines of the form ``<hook> -> event
<name>(<parameters>)``. Let's break it down:

    - ``<hook>`` defines when an event should be triggered, and it
      works just like externally defined BinPAC++ :ref:`hooks
      <pac2_hooks>`, i.e., you give the fully-qualified path to the
      grammar element that is to trigger the event during parsing. In
      the example above, ``on SSH::Banner`` will trigger an event
      whenever an instance of the ``SSH::Banner`` unit has been fully
      parsed. One can also refer to individual unit fields and
      variables (e.g., ``on SSH::Banner::version``), and the unit-wide
      unit hooks work as well (e.g., ``on SSH::Banner::%init``).

    - ``<name>`` corresponds directly to the name of the event as
      visible in Bro. As you can see, one can (and should) use
      namespaces.

    - The ``<parameters>`` define the event's parameters, both their
      types and their values at the same time. Each parameter is a
      complete BinPAC++ expression (except some "magic" ones starting
      with ``$``, see below). The type of the BinPAC++ expressions
      determines the Bro type of the event parameter, with an internal
      mapping defining how a BinPAC++ type translates into a Bro type
      (e.g., a ``bytes`` objects translates into a Bro ``string``; see
      :ref:`pac2_bro-type-mapping` for the details). At runtime, when an
      event is raised, the expression is evaluated to determine the
      value passes to handlers. That evaluation takes place in a hook
      context corresponding to what ``<hook>`` triggers on, and it has
      access to the same scope as a manually written hook. For
      example, with the ``on SSH::Banner`` event, ``self`` refers to
      the unit instance and ``self.version`` to its ``version`` item.
      Or if you wanted, e.g., an upper-case version of the software
      identification, you could use ``self.software.upper()``.

      In addition to that, two "magic" parameters provide access to
      internal Bro state:

        - ``$conn`` references the current connection that's being
          passed, and it translates into a ``connection`` record
          parameter for the corresponding Bro event.

        - ``$is_orig`` turns into a boolean value indicating whether
          BinPAC++ is parsing the originator or responder side of the
          connection.

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

To check that Bro indeed finds the new analyzer, check the ``bro -NN``
output::

    # bro -NN
    [...]
    Plugin: Bro::Hilti - Dynamically compiled HILTI/BinPAC++ functionality (dynamic, version 1)
        [...]
        [Analyzer] pac2_SSH (ANALYZER_PAC2_SSH, enabled)
        [Event] ssh::banner
        [...]
    [...]

We see Bro has found the BinPac++ plugin, and that one now provides a
new ``pac2_SSH`` analyzer with one event ``ssh_banner``. Let's try it
with a trace containing a single SSH connection::

    # bro -r ssh-single-conn.trace ssh-banner.bro
    SSH banner, [orig_h=192.150.186.169, orig_p=49244/tcp, resp_h=131.159.14.23, resp_p=22/tcp], F, 1.99, OpenSSH_3.9p1
    SSH banner, [orig_h=192.150.186.169, orig_p=49244/tcp, resp_h=131.159.14.23, resp_p=22/tcp], T, 2.0, OpenSSH_3.8.1p1

HILTI/BinPAC++ Options
----------------------

Bro's BinPAC++ comes with a number of options defined in
``hilti2/bro/scripts/bro/hilti/base/main.bro``. These include:

``debug: bool`` (default: false)
    If true, compiles all BinPAC++/HILTI code in debug mode, meaning
    that the :ref:`pac2_debugging` will be
    available (including the ``HILTI_DEBUG`` environemnt variable).

``dump_debug: bool`` (default: false)
    Dumps debug information about the compiled analyzers to the
    console. Can be helpful if Bro isn't integrating a BinPAC++
    analyzer as expected.

``optimize: bool`` (default: true)
    Activates code optimization. Should be on normally, but makes
    debugging in the debugger easier if off.

``use_cache: bool`` (default: true)
    Enables caching of compiled BinPAC++/HILTI code. If on, you will
    notice that the first time you start Bro with a new (or modified)
    analyzer, it takes signficantly longer that on subsequent
    executions. That's because the first needs to do all the leg-work
    of going from BinPAC++ to native code in the background. It then
    however caches the resulting code on disk and reuses it next time
    directly.

    Normally you want this on, however currently the cache does not
    always pick up on changes made to ``*.pac2`` or ``*.evt`` files
    (or Bro itself, or the BinPAC++ plugin) during execution. If you
    notice that a change doesn't seem to take effect, try removing the
    cache directory (``.cache``) or simply disable it altogether.

See the script itself for the complete list.

.. _pac2_bro-type-mapping:

Type Mapping
------------

The following table summarizes how BinPAC++ types get mapped into Bro
types. (Types not listed are not yet implemented.)

===========    ==========
BinPAC++       Bro
===========    ==========
``addr``       ``addr``
``bool``       ``bool``
``bytes``      ``string``
``double``     ``double``
``int<*>``     ``int``
``string``     ``string``
``uint<*>``    ``count``
===========    ==========

