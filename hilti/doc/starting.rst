
Getting Started
===============

.. contents::

Prerequisites
-------------

There are a number of dependencies that HILTI needs to find
installed:

    * Python 2.6,  http://www.python.org
    * LLVM 2.8, http://llvm.org/releases
    * Clang 2.8, http://clang.llvm.org
    * llvm-py, http://code.google.com/p/llvm-py/downloads/list
    * Ply 3.3, http://www.dabeaz.com/ply
    * CMake 2.8, http://www.cmake.org
    * Flex 2.5.35, http://flex.sourceforge.net/#downloads

If you want to generate the documentation (optional), you also need:

    * Sphinx 1.0.4, http://sphinx.pocoo.org
    * Doxygen 1.7.2, http://www.stack.nl/~dimitri/doxygen
    * Graphviz 2.24.0, http://www.graphviz.org

In addition, there is eusually a number of custom patches required for
some of these prerequisites; they are located in the top-level
``patches/`` directory of the distribution. 

It is of course a pain to assemble all these pieces manually and
then keep them up-to-date as new versions are required. Therefore,
there is a meta-package ``hilti-deps`` available that contains the
source for all the packages listed above, except Python. It also
comes with all required patches and ensures they are applied as
necessary. 

To install ``hilti-deps``, do the following: 
      
      * Download the latest hilti-deps package from 
      
          http://www.icir.org/robin/hilti

        As a general rule: the latest git ``master`` version requires
        the latest ``hilti-deps`` package. 
            
      * Untar into a directory ``<prefix>`` where the compiled
        packages can stay, e.g., under ``/opt``, and then run
        ``make`` in the src/ sub-directory::
      
          > cd /opt 
          > tar xzvf hilti-deps-XXX.tgz
          > cd hilti-deps/src
          > make    

        This will take a while (LLVM/clang take a long time to
        compile...) but once done, all packages will have been
        compiled and installed in ``<prefix>`` (e.g., with the
        example above, you'll find ``clang`` in
        ``/opt/hilti-deps/bin/clang``).

      * For HILTI to find the tools, you have to set the environment
        variable ``HILTIDEPS`` to point to the ``<prefix>``
        directory::

          > export HILTIDEPS=<prefix>

      * If you generally want to adapt your environment to use all
        these tools, run::

          > eval `<prefix>/bin/setup-paths`

        This adapts `PATH` (and others) to include the installed
        tools; it also sets HILTIDEPS so you can skip the previous
        step if you do this.

        Note that the ``setup-path`` only works with a bash.

Installation
------------

* Clone the git repository that contains HILTI:

    > git clone git://envoy.icir.org/binpacpp

  There's some other stuff in this repository as well (in particular
  `BinPAC++ <http://www.icir.org/robin/binpac++>`_ , as the name
  already suggests). HILTI is located in ``binpacpp/hilti``.

.. note:: Eventually, we well split up the repository into individual
   ones for each component. As things are still moving fast, it's
   however easier to keep them together for now.

.. note:: HILTI doesn't have a nice installation framework yet, it's
   best run just right ouf of the repository. This will eventually
   change as things get more stable. 

* First, you need to build ``libhilti`` (the run-time library) and
  some tools::

    > cd binpacpp/hilti
    > make

  There shouldn't be any problems compiling this as long as all
  dependencies have been installed correctly. 

* Next, you should see if the test-suite passes::

     > cd binpacpp/hilti/tests
     > make

  All tests are supposed to succeed. Occasionally, a few
  individual ones may not pass, but if the majority fails,
  there's something wrong with the setup. 

  If there's a problem with a test, ``binpacpp/btest/btest -d
  path/to/test`` will give you debugging output. 

* As the HILTI tools aren't installed anywhere system-wide yet, it's recommend to link
  to them from some directory that's in your ``PATH``, such as::

     > export PATH=$HOME/bin:$PATH
     > ln -s binpacpp/hilti/tools/*/{hiltic,hilti-build,hilti-build2} $HOME/bin
     > ln -s binpacpp/btest/btest $HOME/bin

  In the following, we assume that the tools are found in the
  ``PATH``.

Compiling a HILTI Program
-------------------------

Here's a simple "Hello, World!" in HILTI::

    module Main

    import Hilti

    void run() {
        call Hilti::print ("Hello, World!")
    }

Assuming that's stored in ``hello.hlt``, we can compile it with
|hb| and then run::

    > hilti-build2 -o a.out hello.hlt
    > ./a.out
    Hello, World!

Note that a standalone HILTI module (i.e., a module that's run
directly in this way, not linked into a C host application) must
always have a ``Main::run`` function, which is where execution
starts. 

Use the option to ``-v`` to see what |hb| is doing::

    > hilti-build2 -v -o a.out hello.hlt
    compiling hello.hlt ...
      > parsing HILTI code
      > resolving HILTI code
      > validating HILTI code
      > canonifing HILTI code
      > generating LLVM code
      > validating LLVM code
      > writing LLVM bitcode
    linking a.out
      > llvm-ld -disable-internalize -disable-opt -b=a.out.hb7743.tmp.bc hello.hb7743.tmp.bc [ ... many -L ...] -lbinpac -lhiltimain -ljrx
      > clang -g -O0 -o a.out a.out.hb7743.tmp.bc -lc -lpthread -lpcap

As you can see, |hb| compiles the HILTI code into LLVM
code internally, then writes the result out and uses the LLVM tools
to build the final executable. 

There's another tool |hc| that outputs just the generated LLVM
code (or optionally various intermediary representations)::

   > hiltic -l hello.hlt
   [... lots of hard to read LLVM code on stdout ...]


Exploring More
--------------

* This documentation is found ``binpacpp/hilti/doc``. The text is
  written in *reST* and thus pretty readable as ASCII. To build html
  in ``doc/_build``, just type ``make``.

  Note that the documentation is a work in progress, with more and
  more pieces appearing there over time.

* Look at the HILTI source files (``*.hlt``) in the ``tests/*`` subdirectories to
  see how HILTI programs look like.

* Tools:
    * ``tools/hiltic`` is the HILTI->LLVM compiler
    * ``tools/hilti-build2`` runs the whole chain HILTI-to-native-executable. 





