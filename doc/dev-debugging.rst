
Debugging HILTI
===============

The HILTI code generator can insert additional instrumenation that
helps debugging problems. Likewise, libhilti has a number of debugging
facilities. To activate debugging support, generally one needs to
compile with a debugging level large than zero (|hc|/|hb| options
``-d``) and without optimization (i.e., no ``-O``). Doing so will
automatically link with a debugging version of libhilti.

Debug Output
------------

Memory Leaks
------------

TODO

Note that the objective is to not have *any* leaks reported. The
test-suite automatically activates this leak-checking for all tests
and will fail if any are encountered.

LLVM Bitcode Annotation
-----------------------

