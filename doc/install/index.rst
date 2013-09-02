
============
Installation
============

.. contents::

Prerequisites
-------------

The HILTI framework is being developed on 64-bit Linux only currently;
usage on other platforms is likely to fail. Specifically, MacOS
support is targeted for the near-future, but currently doesn't work.

To compile the framework, you need LLVM 3.3 and Clang 3.3 from
http://llvm.org, along with C++11-compatible standard libraries. This
combo can still be painful to set up; see below for more.

For unit testing:

* BTest (http://www.bro-ids.org/documentation-git/components/btest/README.html)

For generating the documentation:

* Doxygen 1.7.4 (http://www.stack.nl/~dimitri/doxygen)

* Sphinx 1.1 (http://sphinx.pocoo.org)::

    > easy_install sphinx

* Doxylink extension for Sphinx
  (http://packages.python.org/sphinxcontrib-doxylink)::

    > easy_install sphinxcontrib-doxylink

  (TODO: Currently not fully used).

Getting the Code
----------------

Clone the git repository::

    > git clone git://www.icir.org/binpacpp

There's some other stuff in this repository as well; ignore that and
go straight to ``binpacpp/hilti2`` (*not* ``binpacpp/hilti``, that's
an old version).

.. note:: Eventually, we well split up the repository into individual
   ones for each component. As things are still moving fast, it's
   however easier to keep them together for now.

Installing LLVM/clang/libc++
----------------------------

If your OS doesn't come with a full LLVM/clang 3.3 setup that also
includes C++11 standard libraries (which is likely), you'll need to
compile it yourself. This is a bit cumbersome unfortunately as one
needs to compile clang twice: once to boostrap and once with the right
``libc++``. Order of the involved steps is important.

To make this easier, there's a script doing the necessary steps at
http://github.org/rsmmr/install-clang. See the installation
instructions there. In the following we assume that LLVM/clang is
available via ``PATH``.

Compiling the HILTI framework
-----------------------------

.. note:: HILTI doesn't have a nice installation framework yet, it's
   best run just right ouf of the repository. This will eventually
   change as things get more stable. 

Compiling HILTI itself should be straight-forward. Just run make in
the top-level directory::

    > cd binpacpp/hilti2
    > make

If you want to compile the included Bro plugin as well, you also need
to tell ``make`` where your Bro source tree is::

    > make BRO_DIST=/path/to/bro

.. note:: You currently need a special topic branch of Bro to work
   with HILTI/BinPAC++. See :ref:`pac2_bro-plugin` for more
   information.

If everything has worked right, there should now be a binary
``build/tools/hiltic`` afterwards (as well as a few others).

Next, you should see if two simple tests succeed::

     > cd tests
     > make hello-worlds
     all 2 tests successful

If there's a problem, ``diag.log`` will contain debugging output.

Just typing ``make`` in ``tests/`` will run the full test-suite.
Generally, the majority should succeeed. However, as things are still
in flux, some may be expected to fail.

As the HILTI tools aren't installed anywhere system-wide yet, you may
want to link to them from some directory that's in your ``PATH``, such
as::

     > export PATH=$HOME/bin:$PATH
     > ln -s binpacpp/hilti2/build/tools/{hiltic,hilti-config,hilti-prof,binpac++} $HOME/bin
     > ln -s binpacpp/hilti2/build/tools/pac-driver/pac-driver $HOME/bin
     > ln -s binpacpp/hilti2/tools/hilti-build $HOME/bin

In the remainder of this documentation, we assume that these tools are
indeed found in the ``PATH``.
