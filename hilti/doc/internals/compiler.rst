.. $Id$

.. include:: ../include.rst

Compiler
========

.. _internals-python-module:

..todo: Describe the overall HILTI Python module structure and  usage. 

Core Classes
------------

core.block
~~~~~~~~~~

.. automodule:: hilti.core.block
   :members:
   :undoc-members:

core.constant
~~~~~~~~~~~~~

.. automodule:: hilti.core.constant
   :members:
   :undoc-members:


core.function
~~~~~~~~~~~~~

.. automodule:: hilti.core.function
   :members:
   :undoc-members:

core.id
~~~~~~~

.. automodule:: hilti.core.id
   :members:
   :undoc-members:

core.instruction
~~~~~~~~~~~~~~~~

.. automodule:: hilti.core.instruction
   :members:
   :undoc-members:

core.location
~~~~~~~~~~~~~

.. automodule:: hilti.core.location
   :members:
   :undoc-members:

core.module
~~~~~~~~~~~

.. automodule:: hilti.core.module
   :members:
   :undoc-members:

core.scope
~~~~~~~~~~

.. automodule:: hilti.core.scope
   :members:
   :undoc-members:

core.type
~~~~~~~~~

.. automodule:: hilti.core.type
   :members:
   :undoc-members:

core.util
~~~~~~~~~

.. automodule:: hilti.core.util
   :members:
   :undoc-members:

core.visitor
~~~~~~~~~~~~

.. automodule:: hilti.core.visitor
   :members:
   :undoc-members:

Parser 
------

.. automodule:: hilti.parser

The main external function provided by the :class:`~hilti.parser` is:

.. autofunction:: hilti.parser.parse

Checker
-------

.. automodule:: hilti.checker

The main external function provided by the :class:`~hilti.checker` is:

.. autofunction:: hilti.checker.checkAST

Internals
~~~~~~~~~

.. automodule:: hilti.checker.checker
   :members:

Printer
-------

.. automodule:: hilti.printer

The main external function provided by the :class:`~hilti.printer` is:

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

.. automodule:: hilti.canonifier.module
.. automodule:: hilti.canonifier.flow

Internals
~~~~~~~~~

.. automodule:: hilti.canonifier.canonifier
   :members:

Codegen
-------

.. automodule:: hilti.codegen

The main external function provided by the :class:`~hilti.codegen` is:

.. autofunction:: hilti.codegen.generateLLVM

Internals
~~~~~~~~~

.. automodule:: hilti.codegen.codegen
   :members:




