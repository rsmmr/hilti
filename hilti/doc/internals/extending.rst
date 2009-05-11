.. $Id$

Extending [Mostly missing]
==========================

.. todo: 

This is for the most part a brief list of keywords for now; once
things have stabilized we should turn it into some real text. 

.. _adding-instructions:

Adding a new HILTI instruction
------------------------------

.. _signature-constraints:

Signature Constraints
~~~~~~~~~~~~~~~~~~~~~

When defining an instruction's signature via the ~~signature
decorator, one needs to specify what kind of arguments the
instruction accepts. This is done by associating a *constraint
function* with each used operand (including the target operand),
which then during runtime will check whether an operand looks the
way it is supposed to. 

A constraint function is a Python function that is decorated with 
~~constraint and it must take three parameters: a
~~Type *t*, an ~~Operand *op* and an ~~Instruction *i*. *t* and *op*
are the type and the operand that should be checked, respectively.
*t* will always passed into the function but *op* can be Null if
we're actually not checking an operand but just a type (e.g.,
because we're verifying a type that is part of another type, like
inside a tuple). As a rule of thumb: if the the constraint enforces
a condition on a *type*, it should check *t*; if it requires a property
for an *operand*, it should check *op* and report failure if it's not
set. Finally, *i* is the instruction we're currently checking; it's
always set. 

The constraint function must return a tuple ``(bool, string)``. The
boolean in that tuple indicates whether the operand conforms with
whatever the constraint function checks for. If it does not, the
string gives an error message describing what's wrong in a form
suitable to use in a compiler error presented to the user. If the
boolean is True, the string will be ignored and can be set
arbitrarily. 

The ~~constraint decorator itself must come with one argument: a
ReST string describing the constraint's role in a way suitable to
use in the :ref:`instructions` in the place of operators having that
constraint. 

Here's an example of a simple constraint function that ensures that
an operand is an integer constant::

    import instruction
    import signature
    import type

    @signature.constraint("const-integer")
    def intConstant(ty, op, i):
        """A constraint function that ensures that an operand is an integer constant."""
        if not op or not isinstance(op, instruction.ConstOperand):
            return (False, "must be a constant operand")
        
        return (isinstance(ty, type.Integer), "must be an integer constant")
    
Now we could define an instruction taking two integer constants like
this::

    @instruction("my.instruction", op1=intConstant, op2=intConstant)
    class MyInstruction:
         [...]
         
In the instruction reference, this instruction will then show up as
taking two ``const-integer`` arguments.          

The constraint functions associated with an instruction are used in
two places: 
    
    1. The ~~Checker verifies an |ast|'s correctness by making sure that all
       instruction fit with the constraints imposed by their signatures.
       
    2. For ~~Operators, the overloading mechanism finds the correct
       implementation by searching for an operator with a matching
       signature (of which there must be exactly one). 
    
The module ~~constraints comes with a number of predefined
constraint functions. In particular, there is a constraint function
for most HILTI types that ensures that an operand's type is of the
corresponding class. By convention, the name of the constraint
function is the lower-cased name of the type class (e.g., the
constraint function ``integer`` makes sure an operand is of type
~~Integer.). 

There's one further capability of constraint functions: they can
transparently adjust the type of an operand if it doesn't match the
constraint directly. This is achieved by decorating the constraint
function with an additional ~~refineType decorator. This decorator
takes a single parameter, which is a function performing the type
adjustment. This adjustment function takes two paramters: the
~~Operand *op* we're working on and the ~~Instruction *i* the
operand is part of. It will be called *before* the constraint
function that it is defined for and can adjust the operand *in
place*. 

The canonical example for using a refinement function is associating
a width with an integer constant. The predefined
~~refineIntegerConstant function sets the width of a constant
integer operand to a given number of bits if the operand's value is
within the right range.  

Refinement functions should work silently: they should check whether
their refinement applies to the given operand and if so proceed with
adjusting the operand. If their refinement however does not apply to
the operand, they must leave the operand untouched and silently
return. 

Adding a new HILTI data type
----------------------------

Adding a new data type involves modifying and extending a few places
but usually the necessary changes are pretty much isolated from any
existing functionality. Generally, you should follow the following
steps. Feel free to copy, paste, & adapt code from already existing
data types where appropiate. 

1. In ``hilti/core/type.py``, derive a new type class from the appropiate
   base class of `.type.Type`. Usually, that the base class will be
   either `.type.ValueType` or `.type.HeapType`. The new class must
   have class-variables ``_name`` and ``_id`. The former is a string
   describing the type in way suitable for use in error messages;
   usually ``_name`` corresponds to the type's name in HILTI
   programs. ``_id`` is an integer unique across all types. To this
   end, add a new ``__HLT_TYPE_*`` constant to |hilti_intern.h|,
   picking the next available number and use the same value with
   ``_id``.
   
2. Add the type's HILTI name to ``_keywords`` in ``core/type.py``.
   The name given there will become a new keyword of the HILTI
   language, and should usually match with the type class' ``_name``
   attribute. 

3. Add a constraint function for the new type add the end of
   ``hilti/core/constraints.py``. Just follow the scheme of other
   type's there. 

4. Create a new sub-module in ``hilti/instructions``, defining the
   instructions and operators the type provides. The module must be
   added to ``hilti/instructions/__init__.py``. Set the module's
   doc-string to just a section heading with the type's name; this
   heading will start the type's section in the instruction
   reference. Furthermore, define a module-level variable
   ``_doc_type_description`` and set it to a string describing the
   type in a way suitable for the user-manual's *type* reference;
   include a specification of how values of your type are
   default-initialized if they are not specifically set otherwise
   (see the implementation of default value below). If a HILTI
   programmer can create constants of your new type in its program
   (see discussion of constants below), include a description of the
   constants' syntax in the description as well. 
   
5. Add instructions and operators to your new sub-module; see
   :ref:`adding-instructions` for more information.
   
6. Optionally, if you need particular correctness checks that aren't
   straight-forward to do with :ref:`signature-constraints`, then
   create a sub-module in ``hili/checker`` and add them there. Add
   the new module to ``hilti/checker/__init__.py``.

7. ctor rexpressions, parser

8. Create a new sub-module in ``hilti/codegen``, implementing the
   code-generation for your new instructions and operators. Add the
   new module to :file:`python/hilti/codegen/__init__.py`. 

   Follow these steps for your code-generator:

   - Define a module-level variable ``_doc_c_conversion`` and assign
     it a string that describes how your new type will be
     represented at the C level (i.e., in function call arguments
     and return values). 
   
   - Define a function to return type information for your type in
     the form a suitably initialized ~~TypeInfo object.  The
     function must be decorated with ~~typeInfo. 
     
   - Define a function that returns the LLVM type that should be
     used internally to represent an instance of the new type. If
     your type is a ~~HeapType, this must be a pointer type. The
     function must be decorated with ~~llvmType.
     
   - If your type is a ~~ValueType, you must define a function that
     returns an LLVM value suitable for initializing instances with
     a default value. (Obviouslu, this value should obviously with
     what you specificy in ``_doc_type_description``, see above).
     The function must be decorated with ~~llvmDefaultValue.
     
   - If you extended the HILTI syntax with type-specific
     constructor-expressions (see above), you must define a function
     that turns a corresponding ~~Operand into an LLVM value. The
     function must be decorated with ~~llvmCtorExpr.
     
   - Finally, create a visitor function for each new
     instruction/operand; see :ref:`adding-instructions` for more
     information.
   
   
XXX TODO: make a pass over the rest here

- Add @unpack. docstring should documents the ``Hilti::Packed`` values. 

- Add run-time type information (RTTI) in :file:`libhilti`:

  * Create a new file ``my_type.c`` that defines a function returning a
    string-representation of the type:

    .. code-block:: c

        const __hlt_string* __hlt_my_type_to_string(const __hlt_type_info* type, const void* obj, int32_t options, __hlt_exception* exception)
        {
            // Create a string representation of val.
        }

  * Optionally, define other type conversions in a similar way.

  * Add an external function declaration of ``__hlt_my_type_to_string`` to :file:`libhilti/hilti_intern.h`.

  * Add a corresponding HILTI declaration to :file:`libhilti/hilti_intern.hlt`:

    .. code-block:: c

        declare "C-HILTI" string my_type_to_string(<llvm_type> n, int32 options)

  * Add your new type to the ``COBJS`` variable in :file:`libhilti/Makefile`.

- Create a suite of tests for the new type in :file:`tests/my_type/`.

Constants
~~~~~~~~~

If you want to support constants for your new type in HILTI, you need to extend
the :mod:`~hilti.parser`:

- Add syntax for your constants to :file:`parser/lexer.py`.

- Add an ``p_operand_<type>`` rule in :file:`parser/parser.py`


Adding a ValueType
~~~~~~~~~~~~~~~~~~~~

* ~~ValueTypes will be copied by value. Make sure that that works
  for your type. (In rare cases, a ValueType can internally be
  allocated on the heap and be represented by a pointer; that's for
  example the case for strings because they are of variable length.
  Even in this case, they should however have copy-by-value
  semantics and be non-mutable.)

Adding a HeapType
~~~~~~~~~~~~~~~~~

TODO.

Adding an Operator
~~~~~~~~~~~~~~~~~~

TODO.
