
.. _bro-plugin:

=============
Bro Interface
=============

.. contents::


BinPAC++ comes with a Bro plugin that adds support for ``*.pac``
grammars to Bro. The following walks through installation and usage.
Keep in mind that everything is in development and not necessarily
stable right now.

Installation
============

You currently need a topic branch of Bro that adds support for
dynamically loaded plugins. To get it, use git::

    > git clone --recursive git://git.bro.org/bro
    > git checkout topic/robin/dynamic-plugins

Now you need to build Bro using the same C++ compiler that you compile
HILTI/BinPAC++ with. If that's, say, ``/opt/llvm/bin/clang++``, do:: 

    > CXX=/opt/llvm/bin/clang++ ./configure && make

Then compile HILTI/BinPAC++, pointing it to your Bro clone::

   > make BRO_DIST=/path/to/where/you/cloned/bro/into

Watch the output, it should indicate that it has found Bro.

If things went well, the Bro tests should now succeed::

   > cd bro/tests && make
   [...]
   all XXX tests successful

Cool.

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

TODO.

Analyzer Interface
------------------

We use a ``*.evt`` file to tell Bro two things about our new
analyzers: where to hook it into the traffic processing, and which
events to generate. For the former, we write an ``analyzer`` section
and for the latter a series of ``event`` mappings.

Analyzer Section
^^^^^^^^^^^^^^^^

TODO.

Event Mappings
^^^^^^^^^^^^^^

TOOD.

