.. $Id$

Extending [Mostly missing]
==========================


Adding a new HILTI instruction
------------------------------

Adding a new HILTI data type
----------------------------

Adding a new data type involves modifying and extended different HILTI
components. 

- Derive a new class for the appropiate sub-class of `.type.Type`.  Usually,
  that will be either `.type.StorageType` or `.type.HeapType` in
  :file:`python/hilti/core/type.py`. Add the parser keyword to the ``_keywords``
  list.
  
- Create a :mod:`~hilti.checker` module  with the new type's correctness checks.
  Add the module to :file:`python/hilti/checker/__init__.py`.

- Create an :mod:`~hilti.instruction` module with the new type's instructions.
  The module's doc string should contain a high-level description of the type
  suitable for inclusion into the user manual. Include a description of the
  type's syntax for constants there if appropiate. Add the module to
  :file:`python/hilti/instructions/__init__.py`.

- Create a :mod:`~hilti.codegen` module with the new type's code generation. Add
  the module to :file:`python/hilti/codegen/__init__.py`. In the module:
  
  * Decorate a function with ~~makeTypeInfo to initialize the types ~~TypeInfo.
    
  * Decorate a function with ~~convertCtorValToLLVM if you want to support
    constants for your type (see below).
    
  * Decorate a function with ~~convertTypeToLLVM if it's a ~~StorageType. 
    
  * Decorate a function with ~~convertTypeToC if it's a
    ~~StorageType. Add a docstring to the decorated function which
    explains how the the type is mapped to C; the docstring will
    show up in the documentation automatically.
    
  * Create a visitor for each of the new type's instructions

- Add run-time type information (RTTI) in :file:`libhilti`:

  * Create a new file ``my_type.c`` that defines a function returning a
    string-representation of the type:

    .. code-block:: c

        const __hlt_string* __hlt_my_type_fmt(const __hlt_type_info* type, void* obj, int32_t options, __hlt_exception* exception)
        {
            // Create a string representation of val.
        }

  * Add an external function declaration of ``__hlt_my_type_fmt`` to :file:`libhilti/hilti_intern.h`.

  * Add a corresponding HILTI declaration to :file:`libhilti/hilti_intern.hlt`:

    .. code-block:: c

        declare "C-HILTI" string my_type_fmt(<llvm_type> n, int32 options)

  * Add your new type to the ``COBJS`` variable in :file:`libhilti/Makefile`.

.. todo:: Update the previous paragraph for the changes in interface.

- Create a suite of tests for the new type in :file:`tests/my_type/`.

Constants
~~~~~~~~~

If you want to support constants for your new type in HILTI, you need to extend
the :mod:`~hilti.parser`:

- Add syntax for your constants to :file:`parser/lexer.py`.

- Add an ``p_operand_<type>`` rule in :file:`parser/parser.py`


Adding a StorageType
~~~~~~~~~~~~~~~~~~~~


Adding a HeapType
~~~~~~~~~~~~~~~~~

