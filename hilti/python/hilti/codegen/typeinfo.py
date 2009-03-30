# $ID$

from hilti.core import *
from hilti import instructions

class TypeInfo(object):
    """Represent's type's meta information. The meta information is
    used to describe a type's properties, such as hooks and
    pre-defined operators. A subset of information from the TypeInfo
    objects will be made available to libhilti during run-time; see
    ``struct __hlt_type_information`` in
    :download:`/libhilti/hilti_intern.h`. There must be one TypeInfo
    object for each ~~HiltiType instance, and the usual place for
    creating it is in a type's ~~typeInfo.
    
    A TypeInfo object has the following fields, which can be get/set directly:

    *type* (~~HiltiType):
       The type this object describes.
    
    *name* (string)
       A readable representation of the type's name.
       
    *c_prototype* (string) 
       A string specifying how the type should be represented in C function
       prototypes.

    *args* (list of objects)
       The type's parameters. 
       
    *to_string* (string)
       The name of an internal libhilti function that converts a value of the
       type into a readable string representation. See :download:`/libhilti/hilti_intern.h`
       for the function's complete C signature. 
       
    *to_int64* (string) 
       The name of an internal libhilti function that converts a value of the
       type into an int64. See :download:`/libhilti/hilti_intern.h` for the
       function's complete C signature. 
       
    *to_double* (string)
       The name of an internal libhilti function that converts a value of the
       type into a double. See :download:`/libhilti/hilti_intern.h`
       for the function's complete C signature. 
       
    *aux* (llvm.core.Value)
       A global LLVM variable with auxiliary type-specific information, which
       will be accessible from the C level.
              
    t: ~~HiltiType - The type used to initialize the object. The fields
    *type*, *name* and *args* will be derived from *t*, all others are
    initialized with None. 
    """
    def __init__(self, t):
        assert isinstance(t, type.HiltiType)
        self.type = t
        self.name = t.name()
        self.c_prototype = ""
        self.args = t.args()
        self.to_string = None
        self.to_int64 = None
        self.to_double = None
        self.aux = None
