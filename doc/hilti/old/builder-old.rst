
Builder Interface
=================

.. Test link to Doxygen: :hltc:`CodeGen`.
.. Test link to Doxygen: :hltc:`codegen::CodeGen`.
.. Test link to Doxygen: :hltc:`hilti::codegen::CodeGen`.
.. Test link to Doxygen: :hltc:`hilti::codegen::CodeGen::CodeGen`.


HILTI comes with a C++ interface for constructing ASTs in memory, to
then compile them into into LLVM bytecode directly from there without
the need to go through intermediary files. The builder interface
consistent of the three main components:

    `hltc:`hilti::builder::ModuleBuilder`
        A class encapsulating a module's AST for building it
        incrementally in memory. The class provides convenience
        functions for creating the module's elements such global
        variables and functions.

    `hltc:`hilti::builder::BlockBuilder`
        A class that encapsulates building the AST for a basic HILTI
        block (i.e., a sequence of instructions with exactly one
        terminator at the end). The class provides convenience
        functions for adding elements to the block, like for creating
        instructions and local variables.

    `hltc:`hilti::builder::nodes`
        A set of static functions to create AST nodes such as
        expressions and types.

Refer to the class' documentation for more information on the
functionality they provide.

The builder interface is the *recommended* way to build HILTI ASTs.
Generally, it provides primarily convenience functionality and in
principle, one could also create all the nodes directly. However,
linking them into the right AST structure requires care and boiler
plate code; the builder interface has that knowledge built in.
Furthermore, we aim to keep the builder interface mostly stable across
HILTI versions; if other parts of the HILTI codebase change, the
builder code can often adapt internally while keeping its API
backwards compatible.

Internally, the HILTI parser relies fully on the builder interface as
well. Indeed, the parser's source code in ``hilti/parser/parser.yy``
also acts a reference on how create AST components; take a look there
if you can't figure out how to do something.

.. note::

    The parser does a couple things slightly different than one would
    in C++ code that builds an AST directly. This concerns primarily
    the way how block structures are created (which the parser does in
    two separate steps, vs. the one simpler step we show below for
    function bodies).

Example
-------

Let's build a simple "Hello World" program. Here's the program we want
to arrive at, written manually:

.. literalinclude:: examples/builder-hello-world.hlt

The following code shows how to recreate this in C++.

.. literalinclude:: examples/builder-hello-world.cc

Let's compile this and run::

    > clang++  `hilti-config --hilti-cxxflags --hilti-ldflags` hello-world.cc -o a.out
    > ./a.out

This gives the following output:

.. literalinclude:: examples/builder-hello-world-output.log

Well, that looks good. The example above just prints the generated
HILTI module. However, turning it into LLVM bitcode takes just one
more step:

.. literalinclude:: examples/builder-hello-world-compile.cc

From here, the easiest way to get to an actual executable is using
``hilti-build``::

    > hilti-build hello-world.bc -o hello-world
    > ./hello-world
    Hello, world!

.. note::

   One can also perform this ``hilti-build`` step directly from C++ by
   using corresponding HILTI and LLVM API functionality. We skip that
   here for brevity. Take a look at the ``hiltic`` source to see how
   it links the resulting code with ``libhilti``, which is the main
   missing step on the HILTI side. (``hilti-build`` runs ``hiltic``
   internally. However, ``hilti-build`` still leaves the final step of
   creating the actual binary to ``clang``; it should eventually do
   that directly in memory as well using ``libclang``, instead of
   spawning a separate clang instance as it does currently).

