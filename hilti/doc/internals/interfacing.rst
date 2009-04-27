.. $Id$

.. highlight:: c

.. _interfacing-with-c:

Interfacing HILTI with C
------------------------

HILTI programs can call C functions and vice versa. Most
importantly, this is used by the :ref:`runtime-library` but it works
just the same for any other C code linked into the final native
code. 

.. todo::
    
    Currently, we support only calling C functions *from* HILTI, not
    the other way round. Accordingly, the following is written
    assuming we're calling the C function from HILTI (either via a
    ~~Call statement in an HILTI program, or via an internally
    generated call by the code generator). In the future, C code
    will be able to also call HILTI functions, and then this text
    will likely need some modifications. 

.. _type-conversions:

Type Conversions
~~~~~~~~~~~~~~~~

When a HILTI value is passed to a C function, the type conversion is
pretty straight-forward for most of the primitive types. More
complex types are often internally represented by a ``struct`` of
some sort and are mapped to corresponding pointers. The following
list summarizes how HILTI types are mapped to function parameters
and return values. 

.. include:: ../.auto/cmapping.rst
    
Calling Conventions
~~~~~~~~~~~~~~~~~~~

The HILTI compiler supports two calling conventions (i.e., specifications
describing how parameters and results are passed between caller and callee):
~~C and ~~C_HILTI.

The ~~C convention performs a standard C call according to the target system's
ABI; the parameters given by the caller correspond directly to those received
by the callee (only subject to the type mappings described above); and the
name of the function is left unmodified. The ~~C calling convention therefore
allows to call arbitrary C functions from external libraries or other object
files. It is also the more efficient calling convention of the two assuming it
provides the functionality required. However, the ~~C convention does not
support any HILTI-specific features that don't a direct equivalent in C. In
particular, ~~C functions cannot raise exceptions, receive parameters of type
~~Any, or receive tuple parameters. 

The ~~C_HILTI calling conventions also use the target's ABI to
*implement* the function call, it however modifies the parameters
that the callee receives to support HILTI-specific features.
Specifically, ~~C_HILTI differs from ~~C in the following aspects:
    
    * **Exception Support**. To support exceptions, the callee
      receives an *additional argument*, which is added to the end
      of the parameter list. The argument is of type
      ``__hlt_exception *`` with the type defined in |hilti.h|:
        
      .. literalinclude:: /libhilti/hilti.h
         :start-after: %doc-hlt_exception-start
         :end-before:  %doc-hlt_exception-end

      To raise an exception, the callee sets the additional parameter to an
      exception object and then directly returns from the call (using an
      arbitrary return value, which will be ignored)::
        
          #include <hilti_intern.h>
        
          void foo([...], __hlt_exception *excpt) {
              [...]
              if ( error-condition ) {
                  *excpt = __hlt_exception_value_error;
                  return;
                  }
              [...]
          }
        
      The following standard exceptions are provided:
        
      .. literalinclude:: /libhilti/hilti_intern.h
         :start-after: %doc-std-exceptions-start
         :end-before:  %doc-std-exceptions-end
        
      .. todo:: 
          
          The type used for exceptions as well as the definitions of
          standard exceptions are preliminary and will change in the
          future.
    
    * **Type information**. Most function parameters are statically typed and
      therefore don't need further type information. In some case however the
      type of a parameter is not determined at compile-time and then explicit
      type information is passed into the callee. This happens for: 
          
          - Parameters of type ``any`` (~~Any).
          
          - Parameters of a wild-carded, parameterized type (e.g.,
            "tuple<*>").
          
      Each function parameter that is passed with type information
      is turned into *two* parameters received by the callee: the
      type information and a *pointer* to the actual parameter. The
      latter is of the type as described above. The type information
      is of type ``hlt_type_info-start *``, with the struct defined
      in |hilti_intern.h|:
          
      .. literalinclude:: /libhilti/hilti_intern.h
         :start-after: %doc-hlt_type_info-start
         :end-before:  %doc-hlt_type_info-end
         
      The *type* field identifies the class of the value's type and can have
      one of the following values:
          
      .. literalinclude:: /libhilti/hilti_intern.h
         :start-after: %doc-__HLT_TYPE-start
         :end-before:  %doc-__HLT_TYPE-end
         
      .. todo:: Explain how type parameters are encoded in __hlt_type_info.
      
   * If declared inside a ``module`` scope, the name of the function is
   prefixed with ``<module-name>_``.
          
          
          
          

    

    
    
