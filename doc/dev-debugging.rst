
Debugging HILTI
===============

The HILTI code generator can insert additional instrumenation that
helps debugging problems. Likewise, libhilti has a number of debugging
facilities. To activate debugging support, generally one needs to
compile with a debugging level large than zero (|hc|/|hb| options
``-d``) and without optimization (i.e., no ``-O``). Doing so will
automatically link with a debugging version of libhilti.

Memory Leaks
------------

When running on MacOS, libhilti's ``main()`` function can
automatically run the Apple-provided ``leaks`` tool just before
termination of the process. To activate, set one of the environment
variables ``HILTI_LEAKS`` or ``HILTI_LEAKS_QUIET``. The former will be
a bit more verbose, indicating when ``leaks`` is run. The latter will
only produce output if leaks are indeed found. If so, it will report
the name of a log file with more information, including a stack
backtrace. To make that trace more useful, one should also set
``malloc`` environment variable ``MallocStackLogging``; see
``malloc(1)`` on MacOS.

Note that the objective is to not have *any* leaks reported. The
test-suite automatically activates this leak-checking for all tests
and will fail if any are encountered.

.. todo:: We should provide a script that sets more the ``malloc``
   debugging options and use that with the test-suite as well.

