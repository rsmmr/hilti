.. $Id$

Extending [Mostly missing]
==========================

.. todo: 

This is for the most part a brief list of keywords for now; once
things have stabilized we should turn it into some real text. 

Adding a new HILTI instruction
------------------------------

TODO

- Implementing an existing operator.

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
