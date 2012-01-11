
Execution Model
===============

Module Information
------------------

The HILTI compiler adds a global meta data node
``!hlt.module.<Module>`` to each module, where where ``<Module>`` is
the name of the LLVM module being compiled. The meta data node has the
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
structure :hltc:`__hlt_execution_context`. At runtime, there will be
one instance of this for each virtual thread, plus one global one for
all code executing within the main thread (including all
initialization code).

Global variables declared by HILT modules are a part of this
structure. That means that the size of a context instance depends on
the globals defined in *any* module. We use the the linker to
determine the size, see :ref:`dev-globals`.

The function :hltc:`__hlt_execution_context_new` allocates a new
execution context and intializes all its fields (including the
globals) with their default values. Context objects are memory managed
and when storing references, :hltc:`__hlt_execution_context_ref` and
:hltc:`__hlt_execution_context_unref` must be used. 

.. _dev-globals:

Module Initalization
--------------------

For each module, the HILTI compiler generates a function
``%.hlt.init.module.<Module>()`` that receives all module-global code
(which includes initialization of globals with non-default values).

HILTI's custom linker collects all these initialization functions and
generates an additional function ``__hlt_module_init()`` that calls
the individual ones sequentially (order undefined). This new function
is called from the HILTI run-time at startup.

Globals
-------

HILTI does not have true globals, all state is thread-local. As part
of the :ref:`dev-context` , each thread allocates a memory block large
enough to jointly hold all globals defined in any module. Each
operation involving a global then translates into an access to a
specific offset in the current thread's context.

To make this work, we need linker support for combining all the
globals into a singe data structure. Here's how code generation works
for that:

    * The HILTI compiler lays out the globals for a specific module as
      an struct with corresponding fields. The struct type is called
      ``%hlt.globals.type.<Module>`` and an instance of
      ``%hlt.globals.type.<Module>`` will be part of every
      ``__hlt_execution_context``.

    * All accesses to a global work indirectly by first determining
      the address of the global in memory and then using that for the
      read or write. For getting the address, the code-generator
      generates a function call to a function
      ``%hlt.globals.type.<Module>* %hlt.globals.base.<Module>()``.
      However, this function does not actually exist, and the call
      will be replaced by the linker with code returning the right
      address, based on the current execution context and the module.

    * The HILTI compiler also generates a function ``void
      %.hlt.init.globals.<Module>()`` that initializes all of module's
      globals with default values in an execution context passed in.

    * HILTI's custom linker pass merges information about all globals
      as follows:

        - It build a joint struct from all individual
          ``%hlt.globals.type.<Module>``, called ``%hlt.globals.type``.

        - It determines the total size of ``%hlt.globals.type`` and
          stores it in a global constant ``int64_t
          __hlt_globals_size``.

        - It assigns each module an offset into the
          ``__hlt_execution_context`` for storing its globals, based
          on where that module's ``%hlt.globals.type.<Module>`` is
          located.

        - It traverses each module's AST and replaces all call to
          ``%hlt.globals.base.<Module>()`` with code that returns the
          corresponding base address inside the current
          ``__hlt_execution_context`` (i.e., ``&ctx.globals +
          offset``).

        - It collects all ``.hlt.init.globals.<Module>()`` functions
          and generates an additional ``__hlt_globals_init()`` that
          calls all the individual functions sequentially (order
          undefined). This function is later called by
          :hltc:`__hlt_execution_context_new` to initialize a
          context's set of globals.


Memory Management
-----------------

All objects dynamically allocated by compiled HILTI code (and also
most allocated by libhilti) are managed via reference counting. By
convention, each such object begins with an instance of
:hltc:`__hlt_gchdr`, which stores the reference count information.

In the HILTI compiler's type hierarchy, all ref'counted types are
derived from :hltc:`GarbageCollected`. Note a :hltc:`HeapType` is
always ref'counted, but others may be too (like
:hltc:`hilti::type::String`).

The code generator provides three methods to manage such objects:

    :hltc:`llvmNewObject` 
        Allocates a new instance.

    :hltc:`llvmRef` 
        Indicates that a new reference to an objects has been stored
        (i.e., increasing the reference count).

    :hltc:`llvmUnref` 
        Indicates that a reference to an objects has been deleted
        (i.e., decreasing the reference count). If going to zero, the
        object will have its destructors run (if defined) and then be
        deleted. Conceptually, destruction doesn't need to happen
        immediately though usually it probably will.

There are corresponding functions in ``libhilti`` for internal C-level
memory management (:hltc:`__hlt_object_new`, :hltc:`__hlt_object_ref`,
:hltc:`__hlt_object_unref`). In addition, by convention each garbaged
collected type ``foo`` that provides a function ``foo_new`` also
provides ``foo_ref`` and ``foo_unref``. While equivalent in function
to ``__hlt_object_ref/unref``, calling these is more convinient as
type information doesn't need to be passed in (and it's more readable
too).

For each garbage collected type, HILTI's run-time type information
provides two pieces of information for memory management:

    A destructor function.
        This will be called when a reference count goes to zero. It
        must in turn unref all pointers the instance to be deleted may
        have stored to other collected objects.

    A pointer map.
        A array of offsets specifying where pointers to further
        garbage collected objects are stored inside an instance. This
        i While not strictly necessary for simple ref'counting itself,
        thus will eventually faciliate having a garbage collector
        running in addition to break cycles.

        A pointer map is an array of :hltc:`hlt_ptroffset_t` in which
        each entry gives the offset of a pointer in the allocated
        object (offset counting *includes* the initial
        :hltc:`__hlt_gchdr` instance at offset 0). The end of the
        array is marked by an offset that has the value
        :hltc:`HLT_PTR_MAP_END` , defined in :hltc:`rtti.h`.

        For fully typed-LLVM objects, there is class
        :hltc:`codegen::PointerMap` that computes the pointer map
        automatically at code generation time. For types defined at
        the C-level, ``libhilti`` includes global of the name
        ``__hlt_<type>_ptrmap`` that can be references.

        .. note:: This pointer map stuff is a bit preliminary right
           now, and we don't use the information at all currently. In
           principle we could also use that information to replace the
           destructor function (if we also added type information top
           the map). However, that seems not only less efficient but
           also hard to debug. 


Reference Counting Conventions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[Not implemented yet].

The following conventions are mandatory to follow when working with
reference-counted objects:

    1. When storing a reference in any variable or parameter, first
       *unref* the old value, then *ref* the new value before doing
       the store.

    2. When calling functions of :ref:`dev-cc` ``HILTI`` or
       ``C-HILTI``, we pass all ref-counted objects at +0. For the
       callee the object is guaranteed to exists until it returns, but
       if it needs to store a reference to one of its arguments
       longer, it needs to ref.

    3. When returning an object of a ref-counted type from a
       ``HILTI``/``C-HILTI`` function, we *always* return it at +1.
       I.e., the callee must ``ref`` the object before returning and
       the caller must ``unref`` after receiving it (in practice, the
       caller will normally immediately store the return value so with
       (1) above, it then doesn't need to anything.)

    4. ``HILTI``/``C-HILTI`` functions *unref* all their local
       variables at exit (both normal or exception).

    5. Globals are *unref* when the execution context they are stored
       in gets destroyed.

    6. For functions of other calling conventions
       ``HILTI``/``C-HILTI`` (including internal ones not following
       any standard convention), ref-counting semantics must be
       defined individually (though it's usually best to follow the
       above wherever possible to avoid confusion).

.. note:: These conventions generates more updates than necessary.  We
   should be able to add an optimization pass later that removes a
   number of them.

Exception Handling
------------------

TODO.

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
