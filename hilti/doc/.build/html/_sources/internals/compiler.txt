.. $Id$

.. include:: ../include.rst

Compiler
========

.. _internals-python-module:

..todo: Describe the overall HILTI Python module structure and  usage. 

packages; core vs. the others which have a clear entry function.

Core Classes
------------

core.ast
~~~~~~~~~~

.. automodule:: hilti.core.ast
   :members:
   
core.block
~~~~~~~~~~

.. automodule:: hilti.core.block
   :members:

core.constant
~~~~~~~~~~~~~

.. automodule:: hilti.core.constant
   :members:

core.function
~~~~~~~~~~~~~

.. automodule:: hilti.core.function
   :members:

core.id
~~~~~~~

.. automodule:: hilti.core.id
   :members:

core.instruction
~~~~~~~~~~~~~~~~

.. automodule:: hilti.core.instruction
   :members:

core.location
~~~~~~~~~~~~~

.. automodule:: hilti.core.location
   :members:

core.module
~~~~~~~~~~~

.. automodule:: hilti.core.module
   :members:

core.type
~~~~~~~~~

.. automodule:: hilti.core.type
   :members:
   :undoc-members:

core.util
~~~~~~~~~

.. automodule:: hilti.core.util
   :members:

core.visitor
~~~~~~~~~~~~

.. automodule:: hilti.core.visitor
   :members:
   :undoc-members:

Parser 
------

.. automodule:: hilti.parser

The main external function provided by the :mod:`~hilti.parser` is:

.. autofunction:: hilti.parser.parse

Internals
~~~~~~~~~

.. autoclass:: hilti.parser.parser.State
.. autofunction:: hilti.parser.parser.loc
.. autofunction:: hilti.parser.parser.parse
.. autofunction:: hilti.parser.parser.error

.. todo:: Describe lexer, in particular the two states.

Checker
-------

.. automodule:: hilti.checker

The main external function provided by the :mod:`~hilti.checker` is:

.. autofunction:: hilti.checker.checkAST

Internals
~~~~~~~~~

.. automodule:: hilti.checker.checker
   :members:

Printer
-------

.. automodule:: hilti.printer

The main external function provided by the :mod:`~hilti.printer` is:

.. autofunction:: hilti.printer.printAST

Internals
~~~~~~~~~

.. automodule:: hilti.printer.printer
   :members:

Canonifier
----------

.. automodule:: hilti.canonifier

The main external function provided by the :mod:`~hilti.canonifier` is:

.. autofunction:: hilti.canonifier.canonifyAST

.. List the canonifications performed. 
.. automodule:: hilti.canonifier.module
.. automodule:: hilti.canonifier.flow

Internals
~~~~~~~~~

.. automodule:: hilti.canonifier.canonifier
   :members:

Codegen
-------

.. automodule:: hilti.codegen

The main external function provided by the :mod:`~hilti.codegen` is:

.. autofunction:: hilti.codegen.generateLLVM

Internals
~~~~~~~~~

.. automodule:: hilti.codegen.codegen
   :members:




