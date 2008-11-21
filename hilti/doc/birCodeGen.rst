
.. include:: global.inc

Code Generation
===============

Conventions
-----------

* Identifiers starting with ``$`` are reservered for internal
purposes.



Execution Model
---------------

struct Continuation {
    ref<Label>        func
    ref<Frame>        frame
}

struct CommonFrame {
   ref<Continuation> cnt_normal
   ref<Continuation> cnt_exception
   int               current_exception 
   ref<any>          current_exception_data

- The currently running function is defined by a function_specific Frame:

  struct func_Frame {
      CommonFrame       common
      <T1>              arg_1
      ...               ...
      <Tn>              arg_n
      <Tn+1>            local_1
      ...
      <Tn+m>			local_m
      }

  The name $FRAME is always bound to the Frame of the current
  function invocation.  
  
-   

