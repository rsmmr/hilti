
Getting Started
===============

.. contents::

Prerequisites
-------------

|binpac| needs the `HILTI <http://www.icir.org/robin/hilti/doc>`_
abstract machine installed. Before proceeding, make sure that HILTI
is installed with all of its dependencies, and that the HILTI
test-suite passes with no (or only minor) problems.

Installation
------------

* Checkout the git repository that contains |binpac|::

    > git clone ssh://envoy.icir.org/binpacpp

  There's some other stuff in this repository as well (in particular HILTI, so
  you may already have it checked out). |binpac| is located in
  ``binpacpp/bp++``.

.. note:: The repository is currently private so your SSH key needs
   to be installed on the server. 

.. note:: Eventually, we well split up the repository into individual
   ones for each component. As things are still moving fast, it's
   however easier to keep them together for now.

.. note:: |binpac| doesn't have a nice installation framework yet, it's best
   run just right ouf of the repository. This will eventually change as things
   get more stable. 

* First of all, you need to build ``libbinpac``, the run-time
  library::

    > cd binpacpp/bp++/libhilti
    > make

  There shouldn't be any problems compiling this as long as all
  dependencies have been installed correctly. 

* Next, you should see if the test-suite passes::

     > cd binpacpp/bp++/tests
     > make

  All tests are supposed to succeed. Occasionally, a few
  individual ones may not pass, but if the majority fails,
  there's something wrong with the setup. 

  If there's a problem with a test, ``binpacpp/btest/btest -d
  path/to/test`` will give you debugging output. 

* As the |binpac| tools aren't installed anywhere system-wide yet, it's
  recommend to link to them from some directory that's in your ``PATH``, such
  as::

     > export PATH=$HOME/bin:$PATH
     > ln -s binpacpp/bp++/tools/*/{binpac++,binpac-config,make-pac-driver} $HOME/bin
     > ln -s binpacpp/btest/btest $HOME/bin

  In the following, we assume that the tools are found in the
  ``PATH``.

* Note that you'll also need the HILTI tools in your PATH; in particular
  |hb| is also the driver for building |binpac| parsers.

Compiling a |binpac| Specification
----------------------------------

Here's a simple "Hello, World!" in |binpac|::

    module Test;

    print "Hello, world!"

Assuming that's stored in ``hello.pac2``, we can compile it with |hb| and 
then run it like this::

    > hilti-build2 -o a.out binpacpp/bp++/tools/pac-driver/pac-driver.c hello.pac2
    > ./a.out
    Hello, World!

Note the inclusion of ``pac-driver.c``: |binpac| generated code
cannot run stand-alone but needs a *driver program* providing a
``main`` function as well as normally (though not in this example)
input data for the generated parsers. |pd| is a generic version of
such a driver that can be used for testing and debugging parsers
from the command line, without the need for any further host
application. If you run the generated binary ``a.out`` with
``--help``, you'll see all the options that |pd| provides. 

Also note that the above mini-input does not actually contain any
parser specification; just some global code executed at
initialization time. We'll see later how to write actual parsers
(which however will then be compiled in the same way for stand-alone
command-line usage). 

If you don't want to run the whole compiler tool-chain via |hb|, the
tool |bppp| simply compiles an |binpac| input file into an HILTI
output file::

   > binpac++ hello.pac2
   [... lots of hard to read HILTI code on stdout ...]

Exploring More
--------------

* Continue with the :ref:`overview` to learn more about writing
  parsers. 

* This documentation is found ``binpacpp/bp++/doc``. The text is
  written in *reST* and thus pretty readable as ASCII. To build html
  in ``doc/_build``, just type ``make``.

  Note that the documentation is a work in progress, with more and
  more pieces appearing there over time.

* There are two (preliminary) parsers for HTTP and DNS in
  ``libbinpac/protocols``.

* Look at the |binpac| source files (``*.pac2``) in the ``tests/*``
  subdirectories to see how |binpac| specifications look like. In
  particular, ``test/unit/*.pac`` show the various features
  available for expressing grammars. 

