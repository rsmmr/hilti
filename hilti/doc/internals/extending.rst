.. $Id$

Extending [Mostly missing]
==========================

.. todo: 

This is for the most part a brief list of keywords for now; once
things have stabilized we should turn it into some real text. 

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

Adding a new data type involves modifying and extended different HILTI
components. 

- Derive a new class for the appropiate sub-class of `.type.Type`.  Usually,
  that will be either `.type.ValueType` or `.type.HeapType` in
  :file:`python/hilti/core/type.py`. Add the parser keyword to the ``_keywords``
  list.
  
- Create a :mod:`~hilti.checker` module  with the new type's correctness checks.
  Add the module to :file:`python/hilti/checker/__init__.py`.

- Create an :mod:`~hilti.instruction` module with the new type's
  instructions, and add the module to
  :file:`python/hilti/instructions/__init__.py`. In the module:
  
  * Set the module's doc-string to just a section-heading with the
    type's name. 
  
  * Define a module-level variable ``_doc_type_description``
    containing a string describing the type in a way suitable for
    the user's manual type reference; include a description of the
    type's syntax for constants there if appropiate. 
  
- Create a :mod:`~hilti.codegen` module with the new type's code generation. Add
  the module to :file:`python/hilti/codegen/__init__.py`. In the module:

  * Decorate a function with ~~typeInfo to initialize the type's ~~TypeInfo.
    
  * Decorate a function with ~~llvmCtorExpr if you want the parser
    to support constructor expressions (this is most commonly used
    for constants; see below).
    
  * Decorate a function with ~~llvmType.
    
  * Create a visitor for each of the new type's instructions

  * Define a module-level variable ``_doc_c_conversion`` containing
    a string that describes how the type will be converted to a C
    value for function call to/from C. 

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
