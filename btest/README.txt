.. -*- mode: rst; -*-

.. |date| date::

============================================
BTest - A Simple Driver for Basic Unit Tests
============================================

:Author: Robin Sommer <robin@icir.org>
:Date: |date|
:Version: 0.2

.. contents::

Introduction
============

The ``btest`` is framework for writing unit tests. Borrowing some
ideas from other systems, it's main objective to provide a simple,
straight-forward driver for a suite of tests. Each test consistent
of a set of command lines, which will executed and their exit code
checked. In addition, their output can optionally be compared
against a previously established baseline. 
            
Installation
============

Installation is simple and standard::

   > python setup.py install

This will install two scripts: ``btest``, the main driver program;
and ``btest-diff``, a tool that can be used by tests to compare
output with a previously established baseline. 

Writing a Simple Test
=====================

In the the most simple case, ``btest`` simply executes a set of
commands lines prefixed with ``@TEST-EXEC:``::

  > cat examples/t1.test
  @TEST-EXEC: echo "Foo" | grep -q Foo
  @TEST-EXEC: test -d .
  > btest examples/t1.test
  examples.t1 ... ok

So, both command lines returns success. If one of them didn't, that
would be reported::

  > cat examples/t2.test
  @TEST-EXEC: echo "Foo" | grep -q Foo
  @TEST-EXEC: test -d DOESNOTEXIST
  > btest examples/t2.test
  examples.t2 ... failed

Normally, you would run just all tests found in a directory::

  > btest examples
  examples.t1 ... ok
  examples.t2 ... failed
  1 test failed
  
Why do we need the ``@TEST-EXEC:`` prefixes? Because often the file
containing the tests simultaneously acts as the *input* for
something to run. Let's say we want to validate an ``shell``
script::

  > cat examples/t3.sh 
  # @TEST-EXEC: sh %INPUT
  ls /etc | grep -q passwd
  > btest examples/t3.sh  
  examples.t3 ... ok
  
Here, ``btest`` executes essentially ``sh examples/t3.sh``, and
checks it's return value. The example also shows that the
``@TEST-EXEC`` prefix can appear anywhere, in particular inside the
comment section of another language (here: sh). 

Now, let's say we want to check the *output* of a program, making
sure that it matches what we expect. For that, we first add command
line that produces the output that we want to check, and then run
``btest-diff`` to make sure it matches a previously recorded
baseline. ``btest-diff`` is just another script that returns
success if the output matches, and failure otherweise; so we can use
it inside the test just like we would run another program. In the
follwing example, we use an awk script as a fancy way to print all
files startwith with a dot in the user's home directory. We write
that into a file called ``dots`` and then check whether it matches
what we know from last time::

   > cat examples/t4.awk
   # @TEST-EXEC: ls -a ~ | awk -f %INPUT >dots
   # @TEST-EXEC: btest-diff dots
   /\.*/ { print $1 }

Note that each test, when run, gets its own litte sandbox directory,
so by creating a file like ``dots``, you aren't cluttering up
anything. 

The first time we run this script, we need to record a baseline::

   > btest -U examples/t4.awk
   
Now, ``btest-diff`` has remembered what the ``dots`` file should
look like::

   > btest examples/t4.awk
   examples.t4 ... ok
   > touch ~/.NEWDOTFILE
   > btest examples/t4.awk
   examples.t4 ... failed
   1 test failed
   
If you want to see, what exactly the unexpected change is, there's a
*diff* mode::

   > btest -d examples/t4.awk   
   examples.t4 ... failed
   % 'btest-diff dots' failed unexpectedly (exit code 1)
   % cat .diag
   == File ===============================
   [  ...  current dots file  ... ]
   == Diff ===============================
   --- /Users/robin/work/binpacpp/btest/Baseline/examples.t4/dots
   2010-10-28 20:11:11.000000000 -0700
   +++ dots      2010-10-28 20:12:30.000000000 -0700
   @@ -4,6 +4,7 @@
   .CFUserTextEncoding
   .DS_Store
   .MacOSX
   +.NEWDOTFILE
   .Rhistory
   .Trash
   . Xauthority
  =======================================
   
  % cat .stderr
  
  [ ... if any of the command had  printed something to
  stderr, it would follow here ... ]
  
  Let's delete the new file and we'll be fine again::

  > rm ~/.NEWDOTFILE 
  > btest -d examples/t4.awk 
  examples.t4 ... ok
  
That's already the main functionality the package provides. In the
following, there's a documentation of a set of further options 
extending/modifying this basic approach.
  
Reference
=========

Command Line Options
--------------------

``btest`` must be run with a list of tests and/or directories. In
the latter case, the default is to interpret all files found in any
of the directories (and their sub-directories) to be tests to
execute. It is however possible to include certain files with the
`configuration file`_. ``btest`` further accepts the following
options:

  -U, --update-baseline 
      Records a new baseline for all ``btest-diff`` commands used in
      any of the specified tests. That is, the output they are run
      on is considered authorative and recorded for future runs as
      the version to compare with.
  
  -d, --diagnostics     
      Shows diagnostics for all failed tests. The diagnostics
      includes the command line that failed, its standard error
      output, and potential additional information recorded by the
      command line for diagnostic purposes. In the case of a
      ``btest-diff`` execution, the latter is the ``diff`` between
      baseline and actional output. 
  
  -D, --diagnostics-all
      Shows diagnostics (see above) for all tests, including those
      which pass.
      
  -f DIAGFILE, --file-diagnostics=DIAGFILE
      Shows diagnostics (see above) for all failed tests into the
      given file. If the file exists already, the new output is
      appended.
  
  -v, --verbose
      Shows all test command lines as they are executed.
      
  -w, --wait
      Interactively waits a ``<enter>`` after showing diagnostics
      for a test.
     
  -b, --brief     
      Does not output anything for tests which pass. If all tests
      pass, there will not be any output at all.
  
  -c CONFIG, --config=CONFIG
      Specifies an alternative `configuration`_ file to use. The
      sadasd asd asd default is to use a file called ``btest.cfg``
      if found in the current directory.
  
  -F FILTER, --filter=FILTER
      Activates a filter_ defined in the configuration file. 
  
  -t, --tmp-keep
      Does not delete any temporary files created for running the
      tests (including their output). By default, the temporary
      files for a test will be located in ``.tmp/<test>/``, where
      ``<test>`` the the path to the test's file with all slashes
      replaced with dots. 

``btest`` returns success only if all tests have succesfully passed. 

.. _configuration file:

Configuration
-------------

Specifics of ``btest``'s execution can be tuned with a configuration
file, either ``btest.cfg`` by default if found in the current
directory, or alternatively specificied by the ``--config`` command
line option. The configuration file is ``INI-style``, and an example
template comes with the distribution as ``btest.cfg.example`` and
can be adapted as needed. The file has a main section ``btest``
defining most options; an optional section for defining `environment
variables`_; and further optional sections for defining custom
filters_.

Note that all paths specified in the configuration file are relative
the the ``btest``'s *base directory*. The base directory is either
the one where the configuration file is located if such is given, or
the current working directory if not. When setting values for
configuration options, the absolute path of this directory is
available via the macro ``%(testbase)s`` (the weird syntax is due to
Python`'s ``ConfigParser`` module ...). 

Also note that when running a test, the current working directory
will be set to temporary sandbox (which will later be deleted). All
paths outside of that sandbox must be specified absolute
(potentially using the ``testbase`` macro). 

Options
~~~~~~~

The following options can be set in the ``btest`` section of the
configuration file:

``TestDirs``
    A space-separated list of directories to search for tests. If
    defined, one doesn't need to specify any tests on the command
    line.
    
``TmpDir``
    A directory where to create temporary files when running tests.
    By default, this is set to ``%(testbase)s/.tmp``.
    
``BaselineDir`` 
   A directory where to store the baseline files for ``btest-diff``.
   By default, this is set to ``%(testbase)s/Baseline``.
 
``IgnoreDirs``
   A space-separated lists of relative directory names to ignore
   when scanning given top-level test directories recursively.
   Default is empty.
   
``IgnoreFiles``
   A space-separated lists of filename globs matching files to
   ignore when when scanning given top-level test directories
   recursively. Default is empty.

.. _environment variables:

Environment Variables
~~~~~~~~~~~~~~~~~~~~~

A special section ``environment`` defines environment variables that
will be propapated to all tests::

     [environment]                                                      
     CFLAGS=-O3
     PATH=%(testbase)s/bin:%(default_path)s
     
Note how ``PATH`` can be adjusted for using local scripts: the
example prefixes it with a local ``bin/`` directory inside the
``btest``'s base, using the predefined ``default_path`` macro to
refer to the ``PATH`` as it is would be set by default.

.. _filter:

Filters
~~~~~~~

Filters are a transparent way to adapt the input to a specific
command in a test before it is actually executed. A filter is
defined by adding a section ``[filter-<name>]`` to the configuration
file. This section must have exactly one entry. The name of that
entry is interpreted as the name of a command which's input is to be
filtered; and the value of that entry is the name of the filter
executable. A filter is run with two arguments representing input and
output files, respectively. Example::

  [filter-cat]
  cat=%(testbase)/bin/filter-cat

When the filter is activated by running ``btest`` with
``--filter=cat``, every time a ``@TEST-EXEC: cat %INPUT`` is found,
``btest`` will first execute (something similar) to ``filter-cat
%INPUT out.tmp``, and subsequently ``cat out.tmp`` (i.e., the
original command with the filter output).  In the simplest case, the
filter could be a no-op in the form ``cp $1 $2``.

.. note::
   There are few limitations to the filter concept currently:
   
   * Filters are *always* fed with ``%INPUT``. We should add a way
     to filter other files as well.
     
   * Commands are only recognized if they are directly starting the
     command line. For example, ``@TEST-EXEC ls | cat >outout``
     would not trigger the filter. 
     
   * Filters are only executed for ``@TEST-EXEC``, not for
     ``@TEST-EXEC-FAIL``.

Writing Tests
-------------

``btest`` scans a test file for lines containing keywords that
trigger certain functionality. Currently, the following keywords are
supported:

``@TEST-EXEC: <cmdline>``
   Executes the given command line and abort the tests if it returns
   an error code other than zero. The cmdline is passed to the shell
   so it can be a pipeline, use redirection, etc.  
   
   There are two macros that can be used in ``<cmdline>``:
   ``%INPUT`` will be replaced with the name of the file defining
   the test; and ``%DIR`` will be replaced with the directory where
   that file is located. The latter can be used to reference further
   files also located there. 
   
   In addition to the environment variable defined in the
   configuration files, there are further ones that are passed into
   the commands:
   
       ``TEST_DIAGNOSTICS``
           A file where further diagnostic information can be saved
           in case a command fails. ``--diagnostics`` will show this
           file. (This is where ``btest-diff`` stored the diff.)
           
        ``TEST_MODE``
           This is normally set to ``TEST``, but will be ``UPDATE``
           if ``btest`` is run with ``--update-baselin``.
  
        ``TEST_BASELINE``
           A filename where the command can save permanent
           information across ``btest`` runs. (This where
           ``btest-diff`` stored its baseline in ``UPDATE`` mode.)

``@TEST-EXEC-FAIL: <cmdline>``
   Like ``@TEST-EXEC``, except that this expects the command to
   *fail*, i.e., the test is aborted when the return code is zero.
   
``@TEST-START-NEXT`` 
   This is short-cut for defining multiple test inputs in the same
   file, all executing with the same command lines. When
   ``@TEST-START-NEXT`` is encoutered, the test file is initially
   considered to end at that point, and all ``@TEST-EXEC-*`` are run
   with an ``%INPUT`` truncated there. Afterwards, a *new*
   ``%INPUT`` is created with everything *following* the
   ``@TEST-START-NEXT`` marker, and the *same* commands are run
   again (further ``@TEST-EXEC-*`` are ignored). The effect is that
   a single file defines actually two tests, and the output simply
   enumerates them::

       > cat examples/t5.sh
        # @TEST-EXEC: cat %INPUT | wc -c >output
        # @TEST-EXEC: btest-diff output
        
        This is the first test input in this file.
        
        # @TEST-START-NEXT
        
        ... and the second.
        
       > ./btest -D examples/t5.sh
       examples.t5 ... ok
         % cat .diag
         == File ===============================
         119
         [...]
    
       examples.t5-2 ... ok
         % cat .diag
         == File ===============================
         22
         [...]

   Multiple ``@TEST-START-NEXT`` can be used to create more than
   two tests per file. 
   
``@TEST-START-FILE <file>``
   This can be used to include an additional input file for a test
   right inside the test file. All subsequent lines will be written
   into the given file (and removed from the test's `%INPUT`) until
   a terminating ``@TEST-END-FILE`` is found. Example::
   
       > cat examples/t6.sh
       # @TEST-EXEC: awk -f %INPUT <foo.dat >output
       # @TEST-EXEC: btest-diff output
       
           { lines += 1; }
       END { print lines; }
       
       @TEST-START-FILE foo.dat
       1
       2
       3
       @TEST-END-FILE
    
       > btest -D examples/t6.sh 
       examples.t6 ... ok
         % cat .diag
         == File ===============================
         3
  
   Multiple such files can be defined within a single test.   
   
   Note that this is only one way to use further input files.
   Another is to store a file in the same directory as the test
   itself, make sure it's ignored via ``IgnoreFiles``, and then
   refer to it via ``%DIR/<name>``.

Canonyfying Diffs 
=================

``btest-diff`` has the capability to filter its input through an
additional script before it compares the current version with the
baseline. This can be useful if certain element in an output are
*expected* to change (e.g., timestamps). The filter can then
remove/replace these with something consistent. To enable such
canonfication, set the environment variable ``TEST_DIFF_CANONFIER``
to a script reading the original version from stdin and writing the
canonfied version to stdout. You would normally set this variable in
a configuration file. 

Todo List
=========

* Parallelize execution of tests.

* Output test results in a machine-readable format that can then be
  parsed and pretty-printed by some frontend tool (potentially
  assembling and aggregating output from different systems).
  
