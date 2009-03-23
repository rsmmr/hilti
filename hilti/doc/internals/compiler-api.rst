.. $Id$

.. _python-api:

Public Compiler Interface
=========================

The Python-based HILTI compiler infrastructure consists of a number
of subpackages residing in the ``hilti.*`` namespace:

:ref:`hilti-core`
   A set of core classes representing mostly elements of a parsed
   HILTI |ast|. These classes are used across all other components. 
                  
:ref:`hilti-parser`
   The *parser* turns HILTI programs from ASCII notation into an
   |ast|.                   

:ref:`hilti-checker`
   The *checker* validates the semantic correctness of a HILTI |ast|.

:ref:`hilti-printer`
   The *printer* converts a HILTI |ast| back into readable ASCII notation.

:ref:`hilti-canonifier`
   The *canonifier* transforms an |ast| into a form suitable for use
   by the code-generator. 

:ref:`hilti-codegen`
   The *code generator* generates LLVM code from an AST. 


In the following, we describe the public interface of these
subpackages. For most of them, the public interface consists of a
single function to access the package's functionality. The only
exception is the ``core`` module, which provides a general set of
classes and utility functions. The internals of the other subpackets
are described in :ref:`python-internals`. 

.. _hilti-core:

``hilti.core``
--------------

The *core* package provides a number of further subpackages, which
we list individually in the following. 

``ast``
~~~~~~~

.. automodule:: hilti.core.ast
   :members:
   
``block``
~~~~~~~~~

.. automodule:: hilti.core.block
   :members:

``constant``
~~~~~~~~~~~~

.. automodule:: hilti.core.constant
   :members:

``function``
~~~~~~~~~~~~

.. automodule:: hilti.core.function
   :members:

``id``
~~~~~~

.. automodule:: hilti.core.id
   :members:

``instruction``
~~~~~~~~~~~~~~~

.. automodule:: hilti.core.instruction
   :members:

``location``
~~~~~~~~~~~~

.. automodule:: hilti.core.location
   :members:

``module``
~~~~~~~~~~

.. automodule:: hilti.core.module
   :members:

``type``
~~~~~~~~

.. inheritance-diagram:: hilti.core.type

.. automodule:: hilti.core.type
   :members:
   :undoc-members:

``util``
~~~~~~~~

.. automodule:: hilti.core.util
   :members:

``visitor``
~~~~~~~~~~~

.. automodule:: hilti.core.visitor
   :members:
   :undoc-members:

.. _hilti-parser:

``hilti.parser``
----------------

.. automodule:: hilti.parser
.. autofunction:: hilti.parser.parse

.. _hilti-checker:

``hilti.checker``
-----------------

.. automodule:: hilti.checker
.. autofunction:: hilti.checker.checkAST

.. _hilti-printer:

``hilti.printer``
-----------------

.. automodule:: hilti.printer
.. autofunction:: hilti.printer.printAST

.. _hilti-canonifier:

``hilti.canonifier``
--------------------

.. automodule:: hilti.canonifier
.. autofunction:: hilti.canonifier.canonifyAST

.. List the canonifications performed. 
.. automodule:: hilti.canonifier.module
.. automodule:: hilti.canonifier.flow

.. _hilti-codegen:

``hilti.codegen``
-----------------

.. automodule:: hilti.codegen
.. autofunction:: hilti.codegen.generateLLVM

