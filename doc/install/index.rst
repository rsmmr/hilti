
============
Installation
============

.. contents::

Prerequisites
-------------

The HILTI framework is developed on 64-bit Linux and MacOS, and will
most likely not work on other platforms currently.

To compile the framework, you need LLVM 3.9 and Clang 3.9 from
http://llvm.org, along with C++11-compatible standard libraries.

For Bro integration:

* Bro 2.5, compiled from source (http://www.bro.org)

For unit testing:

* BTest (http://www.bro-ids.org/documentation-git/components/btest/README.html)

For generating the documentation:

* Sphinx 1.3 (http://sphinx.pocoo.org)::

    > easy_install sphinx

Getting the Code
----------------

Clone the git repository::

    > git clone git://www.icir.org/hilti

There's also a `mirror on github
<http://www.github.com/rsmmr/hilti>`_, which you can browse directly.

Compiling the HILTI framework
-----------------------------

.. note:: HILTI doesn't have any installation framework yet, it's best
   run just right ouf of the repository. This will eventually change.

Compiling HILTI itself should be straight-forward once all
prerequisites are available. Just run make in the top-level
directory::

    > cd hilti
    > make

If you want to compile the included Bro plugin as well, you also need
to tell ``make`` where your Bro source tree is::

    > make BRO_DIST=/path/to/bro/src/distribution

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
     > ln -s
     binpacpp/hilti2/build/tools/{hiltic,hiltip,hilti-config,hilti-prof,binpac-config,binpac++,pac-driver,pac-dump} $HOME/bin
     > ln -s binpacpp/hilti2/build/tools/pac-driver/pac-driver $HOME/bin
     > ln -s binpacpp/hilti2/tools/hilti-build $HOME/bin

In the remainder of this documentation, we assume that these tools are
indeed found in the ``PATH``.

.. _docker:

Docker Image
------------

There's also a `Docker image
<https://registry.hub.docker.com/u/rsmmr/hilti/>`_ available that
comes with all pieces preinstalled (HILTI/Spicy, Bro, LLVM/clang)::

    # docker run -i -t "rsmmr/hilti"
    HILTI 0.3-11
    BinPAC++ 0.3-11
    root@b18c7c5bc7e2:~# cat hello-world.pac2
    module Test;

    print "Hello, world!";
    root@b18c7c5bc7e2:~# pac-driver hello-world.pac2
    Hello, world!
    root@b18c7c5bc7e2:~# bro -NN Bro::Hilti
    Bro::Hilti - Dynamically compiled HILTI/BinPAC++ functionality (*.pac2, *.evt, *.hlt) (dynamic, version 0.1)
    [...]
    root@b18c7c5bc7e2:~# bro hello-world.pac2
    Hello, world!
    root@b18c7c5bc7e2:~# bro -r ssh-single-conn.trace ./ssh-banner.bro ssh.evt
    SSH banner, [orig_h=192.150.186.169, orig_p=49244/tcp, resp_h=131.159.14.23, resp_p=22/tcp], F, 1.99, OpenSSH_3.9p1
    SSH banner, [orig_h=192.150.186.169, orig_p=49244/tcp, resp_h=131.159.14.23, resp_p=22/tcp], T, 2.0, OpenSSH_3.8.1p1

To build the Docker image yourself from the supplied Dockerfile, you
can use the make target::

    # make docker-build

Editors
-------

Syntax highlighting support for ``pac2`` files is available in
Vim and Emacs through the following plugins:

* `vim-spicy <https://github.com/blipp/vim-spicy>`_ for Vim; and

* `emacs-spicy <https://bitbucket.org/ldklinux/emacs-spicy>`_ for Emacs.
