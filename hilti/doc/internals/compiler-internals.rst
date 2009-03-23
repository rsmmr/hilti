.. $Id$

.. _python-internals:

Compiler Internals
==================

The following subsection document the internal interfaces of the
HILTI compiler's subpackages. Note that all of these classes and
functions are supposed to be used only *inside* the corresponding
subpackage, not across them. The packages public interfaces are
described in :ref:`python-api`.

``hilti.parser``
----------------

.. autoclass:: hilti.parser.parser.State
.. autofunction:: hilti.parser.parser.loc
.. autofunction:: hilti.parser.parser.parse
.. autofunction:: hilti.parser.parser.error
.. autoclass:: hilti.parser.resolver.Resolver

.. todo:: Describe lexer, in particular the two states.

``hilti.checker``
-----------------

.. automodule:: hilti.checker.checker
   :members:

``hilti.printer``
-----------------

.. automodule:: hilti.printer.printer
   :members:

``hilti.canonifier``
--------------------

.. automodule:: hilti.canonifier.canonifier
   :members:

``hilti.canonifier``
--------------------

.. automodule:: hilti.codegen.codegen
   :members:
