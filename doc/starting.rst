
Getting Started
===============

.. contents::

Prerequisites
-------------

Currently, HILTI will likely compile only on MacOS. Other platforms
may work with a bit of tweaking, but that hasn't been tried yet.

To compile HILTI, you need the following:

* LLVM and Clang 3.0, potentially even newer development versions from
  SVN/git. (http://llvm.org)

* A development version of LLVM's ``libc++`` (http://libcxx.llvm.org).
  Note that the version coming with MacOS isn't recent enough, it has
  some bugs that have been fixed in the meantime. See installation
  instructions below. Also note that GNU's ``libstdc++`` doesn't seem
  to fully work with clang in C++11 mode yet.

For unit testing:

* BTest (http://www.bro-ids.org/documentation/README.btest.html).

For generating the documentation:

* Doxygen 1.7.4 (http://www.stack.nl/~dimitri/doxygen)

* Sphinx 1.1 (http://sphinx.pocoo.org)::

    > easy_install sphinx

* Doxylink extension for Sphinx
  (http://packages.python.org/sphinxcontrib-doxylink)::

    > easy_install sphinxcontrib-doxylink

Installation 
------------

The following has been tried on Mac OS 10.7 only. YMMV.

Compiling LLVM/clang/libc++
~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is a bit cumbersome because we need to compile clang twice, once
to boostrap and once with the right ``libc++``. Order of these steps
is important:

- Get the latest git versions::

    > git clone http://llvm.org/git/libcxx
    > git clone http://llvm.org/git/llvm.git
    > cd llvm/tools
    > git clone http://llvm.org/git/clang.git
    > cd ../..

- Install LLVM and clang into a separate ``--prefix``. Below we assume
  ``--prefix=/opt/llvm``::

    > cd llvm
    > ./configure --prefix=/opt/llvm --enable-optimized
    > make -j 5
    > make install
    > cd ..

- Now, compile ``libc++``::

     > cd libcxx/lib
     > TRIPLE=-apple- ./buildit    # Lion only. SL is different.
     > cd ../..

  Then copy the library into ``/opt/llvm`` (clang will use it
  automatically if it finds it there at the right spot)::

    > mkdir -p /opt/llvm/lib/c++/v1
    > cd libcxx
    > ( (cd include && tar czvf - . ) | ( cd /opt/llvm/lib/c++/v1 && tar xzvf -) )
    > cp `pwd`/lib/libc++.1.dylib /opt/llvm/lib/libc++.1.dylib
    > ln -sf /opt/llvm/lib/libc++.1.dylib /opt/llvm/lib/libc++.dylib
    > cd ..

- Now we need to recompile LLVM/clang to use the right libc++::

    > cd llvm
    > make clean
    > CXX=/opt/llvm/bin/clang++ ./configure --prefix=/opt/llvm --enable-optimized --enable-libcpp
    > make -j 5
    > make install


Compiling HILTI
~~~~~~~~~~~~~~~

If the above succeeded, compiling HILTI itself should be easy:

* Clone the git repository that contains HILTI::

    > git clone git://envoy.icir.org/binpacpp

  There's some other stuff in this repository as well (in particular
  `BinPAC++ <http://www.icir.org/robin/binpac++>`_ , as the name
  already suggests). HILTI is located in ``binpacpp/hilti2`` (*not*
  ``binpacpp/hilti``, that's an old version).

.. note:: Eventually, we well split up the repository into individual
   ones for each component. As things are still moving fast, it's
   however easier to keep them together for now.

.. note:: HILTI doesn't have a nice installation framework yet, it's
   best run just right ouf of the repository. This will eventually
   change as things get more stable. 

* Then just run make in the top-level directory::

    > cd binpacpp/hilti2
    > make

  If everything works right, there should be a binary
  ``build/tools/hiltic`` afterwards.

* Next, you should see if a simple test succeeds::

     > cd tests
     > make hello-world

  If there's a problem, ``diag.log`` will contain debugging output.

  Just typing ``make`` will run the full test-suite but as things are
  being ported over from the old HILTI compiler, there may currently
  be a lot of tests failing.

* As the HILTI tools aren't installed anywhere system-wide yet, you
  may want to link to them from some directory that's in your
  ``PATH``, such as::

     > export PATH=$HOME/bin:$PATH
     > ln -s binpacpp/hilti2/build/tools/hiltic $HOME/bin
     > ln -s binpacpp/hilti2/tools/{hilti-build,hilti-config} $HOME/bin

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

    > hilti-build -o a.out hello.hlt
    > ./a.out
    Hello, World!

Note that a standalone HILTI module (i.e., a module that's run
directly in this way, not linked into a C host application) must
always have a ``Main::run`` function, which is where execution
starts. 

|hb| is HILTI's compiler driver, but it's not doing much work itself.
Use the option to ``-v`` to see what |hb| is running in turn (shorted
for brevity)::

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
  html in ``doc/build``, just type ``make``. This will also build the
  C/C++ API documentation via Doxygen into ``doc/build/doxygen``.

  Note that the documentation is a work in progress, with more and
  more pieces appearing there over time.

* Look at the HILTI source files (``*.hlt``) in the ``tests/*`` subdirectories to
  see how HILTI programs look like.

* Look at options of |hc| (the HILTI compiler and linker) and |hb|
  (the driver that that runs the whole chain from source to
  executable). Just start them with ``--help``.
  
