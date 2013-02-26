
ToDo
====

This section records specific todo items for HILTI's C++ compiler and
run-time support.

AST Infrastructure
------------------

* We should have two types of IDs, ScopedID and UnscopedID, both
  derived from the same base. Then we can use whatever is appropiate
  (scoped for all declarations, and unscoped for all references).

* We should split type::function::Parameter into two classes, one for
  actual parameters and one for return values.

* The HILTI code doesn't use the changes introduced with BinPAC yet:

    - hilti::Declaration doesn't make use of linkage argument.

    - Redo what common.h includes, vs. the other headers.

    - Use the new scripts for declaring AST nodes, visitor interface,
      instruction classes via macros.

* BinPAC doesn't use the coercer anymore and HILTI shouldn't either,
  or better: we should reorg it so that both can use it again.

C/C++ Code
----------

The Doxygen documentation `tracks todos <doxygen/todo.html>`_ 's that
are marked directly in the code.

Code Cleanup
------------

- Use C++11 initializer lists instead of the many manual ``push_back``.

- Get rid of ``id::pathAsString``. Just ``name`` should do it, which
  will then always return the full path.

- Use ``std::make_shared`` consistently.

- The CompilerContext uses its own debugging facilities to enables
  "streams"; we should switch the logger over to that instead of using
  "levels".

- Now that scopes can associate more than one expression with each
  ID, we should port hooks over to using that.

Links
-----

Some of these could be useful in one way or the other.

- Sunrise DD: A hash table implementation with lock-free concurrent
  access. Looks pretty complex though, and does not make sense to
  use until we have settled on a data model for multi-threading.

  http://www.sunrisetel.net/software/devtools/sunrise-data-dictionary.shtml

- Look at http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for an
  alternative (and extremely small) UTF-8 decoder.

- LGPL Aho-Corasick implementation.

Instruction Set
---------------

- ``int.div`` is signed. We should replace that with ``int.sdiv`` and
  ``int.udiv``.

- callables should support hooks directly.

- callables and hooks aren't integrated into the context/scope scheme
  yet.

- the ``struct`` commands should take normal IDs, rather than strings,
  for field names.

- the "const" markers for instructions arguments aren't used/propagated.

Optimizations
-------------

Here we collect some thoughts on code optimization we could apply at
the HILTI and LLVM levels.

HILTI Level
~~~~~~~~~~~

- Track which fields of a struct are actually needed. Those which are
  not read (or not used at all), can be removed for non exported
  types.

  Potentially, a struct could be empty afterwards, in which case can
  remove it completely, including all references to it.

- Track locals which have the same type but are never used
  concurrently.  They can be merged into a single local.

- There are probably a number of micro-optimizations easy and
  worthwhile doing. Look at generated HILTI code.

- Dead-code elimination, in particular remove all code for hooks which
  are never run.

  While LLVM already does eliminate dead code, doing it at the HILTI
  level as well allows the other optimization above to kick in.

- Inlining at the HILTI level; again, this will allow more
  optimizations to kick in.

- Can we identify cases where we can combine nested structures into
  a single one? Might be hard to do in general, but seems there
  could a few specific cases, particularly coming out of BinPAC,
  where it will be helpful.

- BinPAC++ uses a ``__cur`` field in the parse objects to allow hooks
  to change the current parsing position. Before a hook is run, that
  field is set to the current position and afterwards its value is
  written back to the current position. A hook can change it in
  between. However, most of the time there is no change and the
  compiler should optimize then that field away.

- We currently need the ``__clear`` attribute for function parameters
  as well as the ``clear`` instruction for values to explicitly remove
  no longer necessary memory references that prevent the GC from
  cleaning things up. Seems both of these could be figured out by the
  optimizer automatically so that references are always cleared as
  soon as helpful if it would help with GC.

  In addition, this analysis could also be used to generally better
  use variables once we figured out that their old values won't be
  used anymore. 

- We should add information to instructions which exception they can
  throw. That can simply the CFG quite a bit by removing lots of
  edges we have to add now to account for the fact that all we can say
  is that every instruction may throw any exception.

LLVM Level
~~~~~~~~~~

  - We could examine function calls for whether they can yield. If
    not, we may not need to execute inside a fiber. This 
    would accelerate the top-level C function.

Misc
----

The documentation should be generated under the top-level ``build/``
directory, not in ``doc/build/``.

ToDos Recorded in Sphinx
------------------------

.. todolist::
