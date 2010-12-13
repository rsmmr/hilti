.. $Id$

.. _runtime-env:

Runtime Environment
===================

This section describes specifics of the run-time environment in
which compiled HILTI programs are executed:

* The :ref:`flow-model` describes how flow of execution is
  controlled inside a HILTI program, including convention for
  function calls, as well as the implementation of continuations and
  exceptions.

* The :ref:`memory-management` discusses the use of gargabe
  collection as HILTI's primary way of managing dynamic memory, as
  well as some custom schemes for particular purposes.

Note that many of the implementation details described in this
section are *preliminary* and will very likely change as the HILTI
platform evolves. Many of the current design decisions were
determined with the primary objective being to get something running
quickly. While we want to provide the main HILTI capabilities right
from the beginning so that code can start using them without needing
to be rewritten later, their *implementation* is often not the most
efficient at this time and deliberately left for later refinement.

.. _flow-model:

Flow-Control Model
------------------

One of the main objectives for HILTI's flow-control model is the support of
`continuations <http://en.wikipedia.org/wiki/Continuation>`_: at any time, a
HILTI program must be able to "freeze" its current state and return control to
some other place, with the ability to later resume operation at the same place
without any loss of state. We achieve this by use the following mechanisms:

Heap-allocated function frames.
    The local frames for function executions (which store function
    parameters and local variables) are allocated on the *heap* and
    managed via :ref:`garbage collection <garbage-collection>`.
    Functions are passed a pointer to their frame. This allows us to
    suspend a function at any time without losing its state.

Tranformation into CPS.

   HILTI programs are transformed into `continuation passing style
   <http://en.wikipedia.org/wiki/Continuation-passing_style>`_. This
   transformation includes (1) turning all function calls into `tail calls
   <http://en.wikipedia.org/wiki/Tail_call>`, and (2) passing explicit
   information into each function execution where to continue execution after
   the function has finished. The combination of these two makes us
   independent of the call stack: at the end of a function's execution, we can
   just do a jump to wherever execution should continue.

The implementation currently uses a very simple approach for the CPS
conversion: all of a function's ~~Block objects are transformed so that they
contain exactly one |terminator| instruction, which must be the last
instruction of the block. That guarentees that control-flow cannot be diverted
inside a block. Each block is then transformed into its own function, and all
terminators into tail-calls to the corresponding function. It's not completely
clear to which degree the compiler can optimize the additional overhead of
this simple approach away but in any case there's probably a better way to
implement the tail-call conversion, to be done later.

.. note::

   To stress it again, this execution model is preliminary. We will eventually
   need to see to which degree we make use of continuations and whether some
   restricted form would allow for a better implementation; see Nick's
   thoughts on this.

We now discuss this execution model in more detail, using a
pseudo-notation for readability (rather than LLVM syntax; look at
the generated LLVM output if you want to see how these are mapped to
LLVM ...).

.. automodule:: hilti.codegen.flow

.. _memory-management:

Memory Management [Missing]
---------------------------

.. _garbage-collection:

Garbage Collection [Missing]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Custom Allocators [Missing]
~~~~~~~~~~~~~~~~~~~~~~~~~~~





