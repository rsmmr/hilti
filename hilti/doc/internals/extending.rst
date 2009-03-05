.. $Id$

.. include:: ../include.rst

Extending [Mostly missing]
==========================


Adding a new HILTI instruction
------------------------------


Adding a new HILTI data type
----------------------------

- Derive a new class for the appropiate sub-class of `.type.Type`.
  Usually, that will be either `.type.StorageType` or
  `.type.HeapType`.
  
- Create a module in :file:`instructions/` with the new
  type's instructions. The module's doc string should contain a
  high-level description of the type suitable for inclusion into the
  user manual. Include a description of the type's syntax for
  constants there if appropiate. Add the module to
  :file:`instructions/__init__.py`.

- Create a module in :file:`checker/` with the new
  type's correctness checks. Add the module to
  :file:`checker/__init__.py`.

- Create a module in :file:`codegen/` with the new type's code
  generation. Add the module to
  :file:`codegen/__init__.py`. In the module:
  
  * Decorate a function with ~~makeTypeInfo to initialize the types
    ~~TypeInfo.
    
  * Decorate a function with ~~convertConstToLLVM if you want to
    support constants for your type (see below).
    
  * Decorate a function with ~~convertTypeToLLVM if it's a
    ~~StorageType. 
    
  * Decorate a function with ~~convertTypeToC if it's a
    ~~StorageType and needs a separate type conversion when used as
    parameter for a C function call. 
  
  * Create a visitor for each of the new type's instructions

Constants
~~~~~~~~~

If you want to support constants for your new type in HILTI, you
need to extend the parser:

   - Add syntax for your constants to :file:`parser/lexer.py`.
   
   - Add an ``p_operand_<type>`` rule in :file:`parser/parser.py`


Adding a StorageType
~~~~~~~~~~~~~~~~~~~~


Adding a HeapType
~~~~~~~~~~~~~~~~~

