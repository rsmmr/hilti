
.. include:: global.inc

Code Generation
===============

Conventions
-----------

* Identifiers starting with ``__`` are reservered for internal
purposes.

Data Structures
---------------

* A ``__continuation`` is a snapshot of the current processing state,
consisting of (a) a function to be called to continue
processing, and (b) the frame to use when calling the function.::

    struct Continuation {
        ref<label>         func
        ref<__basic_frame> frame
    }

* A ``__basic_frame`` is the common part of all function frame::

    struct __basic_frame {
        ref<__continuation> cont_normal 
        ref<__continuation> cont_exception
        int                 current_exception
        ref<any>            current_exception_data
    }
    
* Each function ``Foo`` has a function-specific frame containg all
parameters and local variables::

    struct ``__frame_Foo`` {
        __basic_frame  bf
        <type_1>       arg_1
        ...
        <type_n>       arg_n
        <type_n+1>     local_1
        ...
        <type_n+m>     local_m
        

Execution Model
---------------

Calling Conventions and Call Execution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* All functions take a mandatory first parameter
``ref<__frame__Func> __frame``. Functions which work on the result
of another function call (see below), take a second parameter
``<result-type> __return_value``.

* Before code generation, all blocks are preprocessed according to
the following constraints:

  - Each instruction block ends with a *terminator* instruction,
  which is one of ``jump``, ``return``, ``if.else``, or
  ``call.tail.void`, or ``call.tail.result`.
  
  - A terminator instruction must not occur inside a block (i.e.,
  somewhere else than as the last instruction of a block).
  
  - There must not be any ``call`` instruction. 
  
  - Each block has name unique within the function it belongs to.  

* We turn each block into a separate function, called
``<Func>__<name>`` , where ``Func`` is the name of the function the
block belongs, and the ``name`` is the name of the block. Each such
block function gets only single parameter of type ``__frame_Func``.

* We convert terminator instructions as per the following pseudo-code:

``jump label``
  ::
  return label(__frame)
  
``if.else cond (label_true, label_false)``
  ::
  return cond ? lable_true(__frame) : lable_false(__frame)

``x = call.tail.result func args`` (function call /with/ return value)
  ::
  TODO

``call.tail.void func args`` (function call /without/ return value)
  ::
      # Allocate new stack frame for called function.
  new_frame = new __frame_func
      # After call, continue with next block.
  new_frame.bf.cont_normal.label = <sucessor block>
  new_frame.bf.cont_normal.frame = __frame
  
      # Keep current exception handler.
  new_frame.bf.cont_exception.label = __frame.bf.cont_exception.label
  new_frame.bf.cont_exception.frame = __frame.bf.cont_exception.frame
  
      # Clear exception information.
  new_frame.bf.exception = None
  new_frame.bf.exception_data = Null
      
      # Initialize function arguments.
  new_frame.arg_<i> = args[i] 

      # Call function.
  return func(new_frame)
  
``return.void`` (return /without/ return value)
  ::
  return (*__frame.bf.cont_normal.label) (*__frame.bf.cont_normal)

``return.result result`` (return /with/ return value)
  ::
  TODO
