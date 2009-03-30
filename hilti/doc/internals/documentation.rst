.. $Id$

Documentation System [Mostly missing]
=====================================

How to Document What
--------------------

Python API
^^^^^^^^^^

Generally, all parts of the Python API should be thoroughly
documented via  `documentation strings <http://docs.python.org/tutorial/controlflow.html#documentation-strings>`_.
The rule of thumb is that all public identifiers (i.e., those
starting without an underscore) should be augmented with a
doc-string. This includes classes, constants, and methods, as well
as those modules which are supposed to be used from external
parties. For internal sub-modules that are part of another package
we sometimes use the module's doc-string for documentating concepts
and implementation, as specified below. When writing doc-strings,
follow the convections in :ref:`doc-strings`.

.. _doc-strings:

Doc String Conventions
^^^^^^^^^^^^^^^^^^^^^^

When writing doc-strings for these, follow these conventions:

* Start each doc-string ith a brief sentence summarizing the item it
  is describing. The sentence might later appear on its own in the
  documentation, without the remainder of the doc-string, and should
  therefore not require further context for understanding. Do not
  reference any function arguments or other details in the first
  sentence. Keep the sentence short.
  
* After the first sentence, describe the documented item in depth.
  For functions/methods, you can reference arguments where
  necessary, but you don't need to. If you reference any arguments,
  use the markup ``*foo*``. In any case, make sure that the text
  contains enough context for the following arguments summary to be
  comprehensible.
  
* For methods/function, document arguments and return value as
  follows:
  
  - Document arguments in a sequence of lines of the following
    form::
    
        name: type - description
    
    For example::
  
        foo: string - The name of foo.
    
    The "type" is informal, e.g., for a list of strings just write
    "list of strings".
  
  - Document the return value in this form::
  
      Returns: type - description
    
    For example::
  
      Returns: string - The name of bar.   
    
  - If the function/method raises any non-standard exceptions, use::
  
      Raises: type
      
    For example::
    
      Raises: ~~MyNonStandardException
      
* Whenever you mention any other HILTI class or other identifier,
  add a cross-reference. In most cases, it will work to just write,
  e.g., ``~~Foo`` to reference identifier Foo.
  
* If you want to point the reader to a particular observation (like
  an implementation detail, or a current deficiency), write a
  paragraph prefixed with "Note:".
  
* If there's something still to-do for the documented item, write a
  paragraph prefixed with "To-Do:" ("Todo:" will also work).

Packages
--------

For HILTI packages (e.g., ``core``, ``canonifier``, ``codegen``, etc.), we
differiate between documenting the public interface and its
internals.

Public Interface
^^^^^^^^^^^^^^^^

Any package must have an ``__init__.py`` file that comes with a
doc-string describing the purpose of the package. For each package,
add a subsection to ``internals/compiler.rst`` and insert a
corresponding ``automodule`` directive. If the package's primary
public interface is a function right in ``init.py`` (as it's the case
for most of HILTI's packages except ~~core), also include an
``autofunction`` directive for it. You can optionally provide some
more context around the ``auto*`` directives. 

Internals
^^^^^^^^^

For all elements internal to a package, add a subsubsection to the
package's description in ``internals/compiler.rst`` and call it
"Internals". Then include ``auto*`` directives as appropiate.

Documenting Functionality in Specific Packages
----------------------------------------------

Instructions
^^^^^^^^^^^^

Each submodules in ``instructions`` should have a module doc-string
containing just a section heading, e.g.::

   """
   Flow Control
   ~~~~~~~~~~~~
   """
  
This heading will start the module's section in the
:ref:`instructions`.
  
In addition, those submodules representing a type should also
provide a module-level string variable ``_doc_type_description``
containing a string describing the type and its usage suitable for
inclusion into the user manual's :ref:`types`. For example::

   _doc_type_description = """The *bool* data type represents
   boolean values. The two boolean constants are ``True`` and
   ``False``. If not explictly initialized, booleans are set to
   ``False`` initially.
   """

Canonifier
^^^^^^^^^^

We document all canonifications performed by the canonifier in the 
doc-string of the *module* that implements the transformation. Write
the module's doc-string in a form suitable for stand-alone reading,
and include a corresponding ``automodule`` directive into the the
canonifier's description of the *public* package API. The code
performing the canonifications doesn't need any further doc-strings;
use Python comments where further detail is helpful. 

Checker
^^^^^^^

The main :class:`~hilti.checker.Checker` class is documented but the
individual checks don't need any further doc-strings. Add Python
comments where context is necessary for understanding what's going
on.

Printer
^^^^^^^

The main :class:`~hilti.checker.Printer` class is documented but the
individual printing function don't need any further doc-strings. Add
Python comments where context is necessary for understanding what's
going on.

Parser
^^^^^^

In :mod:`~hilti.parser.parser` we document all public
classes/functions which are *not* grammar rules. As the grammar
rules come in doc-strings however, we can't just pull all
identifiers into the documentation but must list those we want to
appear separately with *auto* statements in the parser's
"Internals" section.

Codegen
~~~~~~~

Each subpackage representing a specifc type should provide provide a
module-level string variable ``_doc_c_conversion`` containing a
string describing how the type will be converted to C for use with
external C functions. This string will be included in the section
:ref:`type-conversions`. An example is::

   _doc_c_conversion = """ A ``bool`` is mapped to an ``int8_t``,
   with ``True`` corresponding to the value ``1`` and ``False`` to
   value ``0``.
   """

In :mod:`~hilti.codegen.flow` we document implementation details of the
flow-control model.





    
  
  




