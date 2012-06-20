
Execution Model
===============

Module Information
------------------

The HILTI compiler adds a global LLVM meta data node
``!hlt.module.<Module>`` to each module, where ``<Module>`` is the
name of the LLVM module being compiled. The meta data node has the
following sub-nodes (in this order):

    ``version (i16)``
        Version of the HILTI execution model the compiled module is
        assuming. Currently, always 1.

    ``name (i8*)``
        An ASCIIZ string with the HILTI name of the module.

    ``path (i8*)``
        An ASCIIZ string with the path of the module's source file.

Here, ``<Module>`` corresponds to the name of the generated LLVM
module. A few more ``hlt.*`` globals are created, they are described
in the sections below.

.. _dev-context:

Execution Context
-----------------

All state local to a (virtual) thread is kept in instances of a
structure :hltc:`hlt_execution_context`. At runtime, there will be one
instance of this for each virtual thread, plus one global one for all
code executing within the main thread (including all initialization
code).

Global variables declared by HILT modules are a part of this
structure. That means that the size of a context instance depends on
the globals defined in *any* module. We use the the linker to
determine the size, see :ref:`dev-globals`.

The function :hltc:`__hlt_execution_context_new` allocates a new
execution context and intializes all its fields (including the
globals) with their default values. Context objects are memory managed
and when storing references, :hltc:`GC_CCTOR` and :hltc:`GC_DTOR` (or
their variants) must be used; see :ref:`dev-memory`.

Module Initalization
--------------------

For each module that needs it, the HILTI compiler generates a function
``%.hlt.init.module.<Module>()`` that receives all module-global code,
and it adds it to a global LLVM meta data node ``!hlt.modules.init``.

HILTI's custom linker later collects all these initialization
functions and generates an additional function ``__hlt_module_init()``
that calls the individual ones sequentially (order undefined). This
new function is called from the HILTI runtime through
:hltc:`hlt_init`.


.. _dev-globals:

Globals
-------

HILTI does not have true globals, all state is thread-local. As part
of the :ref:`dev-context` , each thread allocates a memory block large
enough to jointly hold all globals defined in any module. Each
operation involving a global then translates into an access to a
specific offset in the current thread's context.

To make this work, we need linker support for combining all the
globals into a single data structure. Here's how code generation works
for that:

    * The HILTI compiler lays out the globals for a specific module as
      an struct with corresponding fields. The struct type is called
      ``%hlt.globals.type.<Module>`` and an instance of
      ``%hlt.globals.type.<Module>`` will be part of every
      ``hlt_execution_context``.

    * All accesses to a global work indirectly by first determining
      the address of the global in memory and then using that for the
      read or write. For getting the address, the code-generator
      generates a function call to a function
      ``%hlt.globals.type.<Module>* %hlt.globals.base.<Module>()``.
      However, this function does not actually exist, and the call
      will be replaced by the linker with code returning the right
      address, based on the current execution context and the module.

    * The HILTI compiler also generates two functions ``void
      %.hlt.init.globals.<Module>()`` and ``void
      %.hlt.dtor.globals.<Module>()``. The former initializes all of
      module's globals with default values in an execution context
      passed in; the latter destroys all the values when a context is
      deleted. The compiler adds these functions to global global LLVM
      meta data nodes ``!hlt.globals.init`` and ``!hlt.globals.dtor``,
      respectively.

    * HILTI's custom linker pass merges information about all globals
      as follows:

        - It build a joint struct from all individual
          ``%hlt.globals.type.<Module>``, called ``%hlt.globals.type``.

        - It determines the total size of ``%hlt.globals.type`` and
          stores it in a global constant ``int64_t
          __hlt_globals_size``.

        - It assigns each module an offset into the
          ``hlt_execution_context`` for storing its globals, based
          on where that module's ``%hlt.globals.type.<Module>`` is
          located.

        - It traverses each module's LLVM AST and replaces all calls
          to ``%hlt.globals.base.<Module>()`` with code that returns
          the corresponding base address inside the current
          ``hlt_execution_context`` (i.e., ``&ctx.globals + offset``).

        - It collects all ``.hlt.init.globals.<Module>()`` functions
          and generates an additional ``__hlt_globals_init()`` that
          calls all the individual functions sequentially (order
          undefined). This function is later called by
          :hltc:`__hlt_execution_context_new` to initialize a
          context's set of globals.

          Similarly, a joint ``__hlt_globals_dtor()`` is created that
          gets called from ``hlt_execution_context_dtor``.



.. _dev-memory:

Memory Management
-----------------

.. todo:: This is not current anymore.

All objects dynamically allocated by compiled HILTI code (and also
most allocated by libhilti) are managed via reference counting. By
convention, each such object begins with an instance of
:hltc:`__hlt_gchdr`, which stores the reference count information.

In the HILTI compiler's type hierarchy, all ref'counted types are
derived from :hltc:`GarbageCollected`. Note that a :hltc:`HeapType` is
always ref'counted, but others may be too (like
:hltc:`hilti::type::String`).

Unfortunately, having memory managed objects also complicates handling
of non-heap objects if they can store references to any of them (e.g.,
tuples, iterators). For these, we need to *copy constructors* and
*destructors* that increase/decreate the reference count for all
embedded referenced, respectively.

The code generator provides a set of methods to create or delete
copies of objects that need memory management, and they all
transparenlty handle both directly garbage collected objects and other
that may themselves store reference of such (and also those who
don't). Generally, these method should be used whenever
creating/copying/deleting a HILTI value:


    :hltc:`llvmCctor`
        Called after a copy of an object has been created (i.e., for
        heap types, when a new references got created; and for value
        types, when the object was copied). It increases all reference
        counts appropiately.

    :hltc:`llvmDtor`
        Called before a copy of an object is deleted (i.e, the
        reference or the object is no longer used). It dereases all
        reference counts appropiately.

    :hltc:`llvmGCAssign`
        Creates a copy of a value and assigns it to a target location,
        calling ``llvmDtor`` for the old value currently stored at the
        target location and ``llvmCctor`` for the copy.

    :hltc:`llvmGCClear`
        Deletes a value by assigning a null value to its location,
        calling ``llvmDtor`` for the old value.

There is corresponding set of macros in ``libhilti`` for internal
C-level memory management (:hltc:`GC_CCTOR`, :hltc:`GC_ASSIGN`, and
others).

To make this all work, by convention each type ``foo`` requiring a
copy constructor or destructor provides corresponding
functions``foo_cctor`` and ``foo_dtor``. These functions are stored as
part of the run-time type information.

.. note::

    There's a bit of magic here: for not garbabe collected types, we
    store the functions directly. For garbage collected types,
    however, we instead store pointers to functions
    increasing/decreasing their reference count. Only if a reference
    count goes to zero, will their actual dtor function be called (and
    they don't have cctor functions).

For each garbage collected type, we also generate a pointer map. That
map is currently not further used, but may be so in the future if we
add a pointer-chasing cycle detector. We describe it here mainly for
later reference. The map is an array of offsets specifying where
pointers to further garbage collected objects are stored inside an
instance. A pointer map is an array of :hltc:`hlt_ptroffset_t` in
which each entry gives the offset of a pointer in the allocated object
(offset counting *includes* the initial :hltc:`__hlt_gchdr` instance
at offset 0). The end of the array is marked by an offset that has the
value :hltc:`HLT_PTR_MAP_END` , defined in :hltc:`rtti.h`. For fully
typed-LLVM objects, there is class :hltc:`codegen::PointerMap` that
computes the pointer map automatically at code generation time. For
types defined at the C-level, ``libhilti`` should define globals of
the name ``__hlt_<type>_ptrmap`` for types that that can be references
(but it doesn't currently).

Reference Counting Conventions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The following conventions are mandatory to follow when working with
garbage collected objects. We frame it in terms of creating copies of
values using their *cctor* and *dtor* function, as described above.
 
    1. When storing a reference in any location that may persist after
       the current function exits, you need to *cctor* it. Make sure
       to *dtor* the old value first. The :hltc:`codegen::Storer`
       takes care of this, and it can be manually done via
       :hltc:`llvmGCAssign`.

    2. When calling functions of :ref:`dev-cc` ``HILTI`` or
       ``C-HILTI``, we generally pass all objects at +0. For the
       callee, the object is guaranteed to exists until it returns. If
       it needs to store a copy to one of its arguments that exceeds
       that, it needs to *cctor*. For ``C`` function, the default is
       the same, though exception may apply as indicated in comments.

       For non-const parameters, we create local shadow variables in
       the function. These shadow variables are treated like any other
       and they are *ctor*'ed upon initialization.

    3. When returning a value from a ``HILTI``/``C-HILTI`` function,
       we *always* return it at +1. I.e., the callee must *cctor* the
       object before returning and the caller must *dtor* after
       receiving it (in practice, the caller will normally immediately
       store the return value so with (1) above, it then doesn't need
       to anything.) For ``C`` function, the default is the same,
       though exception may apply as indicated in comments.

    4. ``HILTI``/``C-HILTI`` functions *dtor* all their local
       variables (and shadow parameters) at exit (both normal or
       exception).

    5. Globals are *dtor*'ed when the execution context they are
       stored in gets destroyed.

    6. Composite objects storing references to values objects must
       ``cctor`` and ``dtor`` as appropiate.

.. note:: These conventions generates more updates than necessary.  We
   should be able to add an optimization pass later that removes a
   number of them.

Exception Handling
------------------

We do exception handling "manually" by storing the current exception
in the execution context and checking it periodicially when it may
have been set (like after a function call). This includes manual stack
unwinding if we don't find a handler.

.. note:: It's unclear if "real" exception handling would be
   sufficiently more efficient to be worth the effort. That's
   something to decide later.


``C-HILTI`` functions are currently handled differently: they get an
additional exception parameter, which they set to raise one.

.. todo::

    That's somethign we should change. There's no reason the C
    functions can't directly set the exception in the execution
    context as well.

Continuations
-------------

TODO.

Hooks
-----

TODO. (Will be similar to module initialization).

.. _dev-cc:

Calling Conventions
-------------------

TODO.
