
Getting Started
===============

.. contents::

Prerequisites
-------------

HILTI has been only used on MacOS and Linux so far, and the current
version requires a 64-bit OS version.

To compile HILTI, you need the following:

* LLVM and Clang 3.1, potentially even newer development versions from
  SVN/git. (http://llvm.org)

* A development version of LLVM's ``libc++`` (http://libcxx.llvm.org).
  Note that the version coming with MacOS isn't recent enough, it has
  some bugs that have been fixed in the meantime. See installation
  instructions below. Also note that GNU's ``libstdc++`` doesn't seem
  to fully work with clang in C++11 mode yet.

For unit testing:

* BTest (http://www.bro-ids.org/documentation-git/components/btest/README.html)

For generating the documentation:

* Doxygen 1.7.4 (http://www.stack.nl/~dimitri/doxygen)

* Sphinx 1.1 (http://sphinx.pocoo.org)::

    > easy_install sphinx

* Doxylink extension for Sphinx
  (http://packages.python.org/sphinxcontrib-doxylink)::

    > easy_install sphinxcontrib-doxylink

Installation 
------------

Getting HILTI
~~~~~~~~~~~~~

Clone the HILTI git repository::

    > git clone git://www.icir.org/binpacpp

  There's some other stuff in this repository as wellHILTI is located
  in ``binpacpp/hilti2`` (*not* ``binpacpp/hilti``, that's an old
  version).

.. note:: Eventually, we well split up the repository into individual
   ones for each component. As things are still moving fast, it's
   however easier to keep them together for now.

Compiling LLVM/clang/libc++
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is a bit cumbersome because we recent versions of clang and its
accompanying libraries; and we need to compile clang twice: once to
boostrap and once with the right ``libc++``. Order of the involved
steps is important. Eventually, this will hopefully all become easy
and standard, but for now we provide a script that does all the magic.


The script is in ``hilti2/scripts/install-llvm``, and it needs a
*source directory* where it can clone the LLVM repositories into, and
an *installation directory* into which it will install the compiled
LLVM infrastructure (e.g., ``/opt/llvm``)::

    ./scripts/install-llvm --install <source dir> <install dir>

This will take a while.

The script has second mode to later update the LLVM installation but
pulling in changes from the upstream git repositories::

    ./scripts/install-llvm --update <source dir> <install dir>

Note that the script currently pulls in the current development
version of all LLVM components, which can occasionally be unstable.


Compiling HILTI
~~~~~~~~~~~~~~~

.. note:: HILTI doesn't have a nice installation framework yet, it's
   best run just right ouf of the repository. This will eventually
   change as things get more stable. 

Assuming the above succeeded, compiling HILTI itself should be
straight-forward. Just run make in the top-level directory::

    > cd binpacpp/hilti2
    > make

If everything works right, there should be a binary
``build/tools/hiltic`` afterwards.

Next, you should see if a simple test succeeds::

     > cd tests
     > make hello-world

If there's a problem, ``diag.log`` will contain debugging output.

Just typing ``make`` in ``tests/`` will run the full test-suite.
However, as things are still being ported over from the old HILTI
compiler, there may a number of failing tests currently.

As the HILTI tools aren't installed anywhere system-wide yet, you may
want to link to them from some directory that's in your ``PATH``, such
as::

     > export PATH=$HOME/bin:$PATH
     > ln -s binpacpp/hilti2/build/tools/hiltic $HOME/bin
     > ln -s binpacpp/hilti2/tools/{hilti-build,hilti-config} $HOME/bin

In the following, we assume that the tools are found in the ``PATH``.

Compiling a HILTI Program
-------------------------

Here's a simple "Hello, World!" in HILTI::

    module Main

    import Hilti

    void run() {
        call Hilti::print ("Hello, World!")
    }

If we store that in ``hello.hlt``, we can compile it with |hb| and
then execute::

    > hilti-build -o a.out hello.hlt
    > ./a.out
    Hello, World!

Note that a standalone HILTI module (i.e., a module that's run
directly in this way, and not linked into a C host application) must
always have a ``Main::run`` function, which is where execution starts.

|hb| is HILTI's compiler driver, but it's not doing much work itself.
Use the option to ``-v`` to see what |hb| is running internally
(shortneed for brevity)::

    > hilti-build -v -o a.out hello.hlt
      > [...]/hilti2/build/tools/hiltic [...] -b -o a.hb96231.tmp.bc  misc/hello-world.hlt
      > clang -L/Users/robin/lib -g -o a.out a.hb96231.tmp.bc

As you can see, |hb| compiles the HILTI code into LLVM bitcode first
using |hc|, the HILTI command-line compiler. It then uses clang to
produce an executable.

You can use |hc| directly as well, for example to just output the
generated LLVM assembly code to the console::

   > hiltic -l -I /path/to/libhilti hello.hlt
   [... lots of read LLVM code on stdout ...]

Note that you need to give |hc| the path to
``binpacpp/hilti2/libhilti`` so that it can find its library files.

Exploring More
--------------

* This documentation is found ``binpacpp/hilti2/doc``. The main text
  is written in *reST* and thus pretty readable as ASCII. To build
  html in ``doc/build/html``, just type ``make``. This will also build the
  C/C++ API documentation via Doxygen into ``doc/build/html/doxygen``.

  Note that the documentation is a work in progress, with more and
  more pieces appearing there over time.

* Look at the HILTI source files (``*.hlt``) in the ``tests/*``
  subdirectories to see how HILTI programs look like.

* Look at options of |hc| (the HILTI compiler and linker) and |hb|
  (the driver that that runs the whole chain from source to
  executable). Just start them with ``--help``.
  
