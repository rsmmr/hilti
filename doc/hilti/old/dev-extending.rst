
Extending HILTI
===============

.. todo:: Note that this text isn't up to date anymore.

Adding Instructions [Missing]
-----------------------------

You need to make the instruction known to HILTI
``hilti/instructions/`` and add code-generation in ``hilti/codegen/``.

Here're the steps:

    * If your new instruction starts a new semantical group (like all
      instructions associated with a specific type), create a new file
      ``<group>.h`` in ``hilti/instructions/``, and add the file to
      the ``instructions`` variable in
      ``hilti/instructions/CMakeList.txt``.

      Also add a comment header describing the group. For types, add
      blocks ``\type`` with the type's name, followed by a general
      description; ``\ctor <example>`` with examples of creating
      instance of the type if it has constants or ctors; and
      ``\cproto`` with the C type it will be mapped to. The easiest is
      to copy a template from an existing type.

      Also add new file to the code generator in
      ``codegen/instructions``, and make it know to the build system
      by editing ``codegen/instructions/CMakeList.txt``.

    * To define the instruction, add an ``iBegin``/``iEnd`` block to
      the corresponding file in ``hilti/instructions/``:

        - TODO.


    * To implement code generation, add a ``visit()`` method to the
      corresponding group in ``codegen/instructions/``.


Adding Types [Partially Missing]
--------------------------------

You need to extend the AST infrastructure and the code generator.

Extending the AST
~~~~~~~~~~~~~~~~~

The basic steps to add a new type to HILTI's AST infrastructure:

    * Derive a new class from the appropiate base class in
      :hltc:`type.h`. For composite types, make sure to call
      :hltc:`addChild` for each sub-node.

    * Add a :hltc:`visit` method for the new type to
      :hltc:`VisitorInterface` in :hltc:`visitor.h`.

    * Create a new namespace ``your-type`` in ``builder.h`` and add at
      least a ``type(...)`` function that creates an instance of the
      new type.

    * To extend the parser to support your new type, add a rule for
      your new type to ``type`` in in ``parser/parser.yy``. You'll
      likely also need extend to adapt the lexer in
      ``parser/scanner.yy``.

    * Extend the printer in ``passes/printer.{h,cc}`` with a method
      ``visit()`` method for your type that prints it out in a syntax
      that the extendend parser can understand.

    * If expressions of your type need to implicitly coerce to other
      types, add a ``visit(()`` method to ``Coercer`` in
      ``coercer.h``.

If your type is a ``ValueType`` and you want to support constants:

    * Add a new class to ``constant.h``. For composite types, make
      sure to call ``Node::addChild`` for each sub-node.
      
    * Add a ``visit()`` method for the new constant to
      ``VisitorInterface`` in ``visitor.h``.

    * In ``builder.h``, add a ``your-type::create(...)`` function
      that returns a new ``expression::Constant``.

    * In ``parser/parser.yy``, add a rule to ``constant``.

    * Extend the printer in ``passes/printer.{h,cc}`` with a method
      ``visit()`` method for your constant that prints it out in a
      syntax that the extendend parser can understand.

    * If your constants need to support implicit coercion to other
      types, add a ``visit()`` method to ``ConstantCoercer`` in
      ``constant-coercer.h``.

      Generally, the cases where constants coerce to other types
      should be aligned with what's happening for non-constant
      expressios (as defined in by ``Coercer``). In some case,
      however, it can make sense to support additional cases for
      constants.

      Note that if for a type, the ``Coercer`` supports coercion to a
      target type while ``ConstantCoercer`` does not, the code
      generator will still coerce via the former (yet potentially less
      efficiently because it can't optimize for the constant case).

If your type is a ``HeapType`` and you want to support ctor
expressions:

    * XXX Similar to above.


Extending the Code Generator
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The basic steps to add a new type to the LLVM code generator:

    * Add a ``visit`` method to ``TypeBuilder`` in
      ``codegen/type-builder.h``. This method needs to return a filled
      out ``codegen::TypeInfo`` structure.

    * If you support constants, add a ``visit`` method to
      ``codegen::Loader`` in ``codegen/loader.h``.

