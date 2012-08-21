
Installation
============

.. contents::

Prerequisites
-------------

The HILTI framework is being developed on MacOS and Linux currently;
usage on other platforms is likely to fail. Also, it currently
supports 64-bit OS version only.

To compile the framework, you need the following:

* LLVM and Clang 3.1, potentially even newer development versions from
  SVN/git. (http://llvm.org)

* A recent of version of LLVM's ``libc++`` (http://libcxx.llvm.org),
  which likely also means a development version.

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

Getting the Code
~~~~~~~~~~~~~~~~

Clone the git repository::

    > git clone git://www.icir.org/binpacpp

  There's some other stuff in this repository as well; ignore that and
  go straight to ``binpacpp/hilti2`` (*not* ``binpacpp/hilti``, that's
  an old version).

.. note:: Eventually, we well split up the repository into individual
   ones for each component. As things are still moving fast, it's
   however easier to keep them together for now.

Installing LLVM/clang/libc++
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is a bit cumbersome because we need recent versions of clang and
its accompanying libraries. We need to compile clang twice: once to
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

Compiling the HILTI framework
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note:: HILTI doesn't have a nice installation framework yet, it's
   best run just right ouf of the repository. This will eventually
   change as things get more stable. 

Assuming the above succeeded, compiling HILTI itself should be
straight-forward. Just run make in the top-level directory::

    > cd binpacpp/hilti2
    > make

If everything works right, there should be a binary
``build/tools/hiltic`` afterwards (as well as a few others).

Next, you should see if a simple test succeeds::

     > cd tests
     > make hello-world

If there's a problem, ``diag.log`` will contain debugging output.

Just typing ``make`` in ``tests/`` will run the full test-suite.
However, as things are still in flux, some may be expected to fail.
Generally, however, the majority should succeeed.

As the HILTI tools aren't installed anywhere system-wide yet, you may
want to link to them from some directory that's in your ``PATH``, such
as::

     > export PATH=$HOME/bin:$PATH
     > ln -s binpacpp/hilti2/build/tools/{hiltic,hilti-config,binpac++} $HOME/bin
     > ln -s binpacpp/hilti2/tools/hilti-build} $HOME/bin

In the remainder of this documentation, we assume that these tools are
found in the ``PATH``.
