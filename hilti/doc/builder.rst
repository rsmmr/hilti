
Builder Interface 
=================

.. contents::

Overview
--------

HILTI comes with a Python interface for constructing ASTs in memory,
to then compile them into into LLVM bytecode directly from there
without the need to go through intermediary files. In addition to 
Python classes representing AST nodes, there are a number of *Builder*
classes providing helper methods for constructing pieces of an AST.
Specifically, there are:

    :py:class:`hilti.builder.ModuleBuilder`
        Builds a module.

    :py:class:`hilti.builder.FunctionBuilder`
        Builds a function.

    :py:class:`hilti.builder.BlockBuilder`
        Builds an block of instructions.

    :py:class:`hilti.builder.OperandBuilder`
        Builds an instruction operand.

Refer to the class' documentation for more information on the methods
they provide.

Generally, the builder classes provide primarily *convienence*
methods, wrapping tasks often required when building an AST and which,
if done "manually", would take quite a bit of boilerplate code. The
provided functionality is however not complete, and whatever the
builder classes don't offer directly can still be done manually by
creating AST classes directly. (Where appropiate, further methods can
of course be added to the builder classes.)

Even when using the builder methods, you will still need other parts
of the HILTI Python API pieces a well, like when constructing types
and operands. If it's not clear how to construct a particular piece
(e.g., an operand with a constant of particular type), the best way to
find out (given that the documention isn't complete yet) is to look at
HILTI's parser for ``*.hlt`` files; it's in ``hilti/parser.py``. The
parser (necessarily) comes with code for creating all AST elements.
Note though that it doesn't use the builder interface even where it
could.

Example
-------

Let's build a simple "Hello World" program. Here's the program we want
to arrive at, written manually:

.. literalinclude:: examples/builder-hello-world.hlt

The following code shows how to recreate this in Python.

.. literalinclude:: examples/builder-hello-world.py

Running this, we get the following output:

.. literalinclude:: examples/builder-hello-world-output.log

Well, that's pretty close to the manually written one. Notes:

    - Module names are case-insensitive.

    - The ``call`` function takes a boolean as its second argument
      indicating whether a newline should be added. This argument is
      ``True`` by default and we don't need specificy it when building
      the tuple operand.

    - The ``Main::run`` method is always automatically exported
      (meaning it can be called from C via an automatically created
      stub function).

The example above just prints the generated HILTI module. However,
turning it into LLVM code takes just one more step:

.. literalinclude:: examples/builder-compile.py

From here, the easiest way to get to an actual executable is using
``hilti-build2``::

    > hilti-build2 hello-world.bc -o hello-world
    > ./hello-world
    Hello, world!

.. note::

   We could also use the LLVM Python API to run the LLVM tool-chain in
   memory, rather than going through the intermediary bitcode file.
   That's still something to figure though, ``hilti-build2`` doesn't
   do that neither right now. 




