# $Id$
"""
Structures
~~~~~~~~~~
"""

_doc_type_description = """
The ``struct`` data type groups a set of heterogenous, named fields. When
initially created, all fields are unset. A field may have a default value, which
is returned for reads when it is unset. 
"""

from hilti.core.type import *
from hilti.core.instruction import *

@instruction("struct.get", op1=Reference, op2=String, target=Any)
class Get(Instruction):
    """Returns the field named *op2* in the struct referenced by *op1*. The
    field name must be a constant, and the type of the target must match the
    field's type. If a field is requested that has not been set, its default
    value is returned if it has any defined. If it has not, an
    ``UndefinedValue`` exception is raised. 
    """

@instruction("struct.set", op1=Reference, op2=String, op3=Any)
class Set(Instruction):
    """
    Sets the field named *op2* in the struct referenced by *op1* to the value
    *op3*. The type of the *op3* must match the type of the field.
    """
    
@instruction("struct.unset", op1=Reference, op2=String)
class Unset(Instruction):
    """Unsets the field named *op2* in the struct referenced by *op1*. An
    unset field appears just as if it had never been assigned an value; in
    particular, it will be reset to its default value if has been one assigned. 
    """

@instruction("struct.is_set", op1=Reference, op2=String, target=Bool)
class IsSet(Instruction):
    """Returns *True* if the field named *op2* has been set to a value, and
    *False otherwise. If the instruction returns *True*, a subsequent call to
    ~~Get will not raise an exception. 
    """
    

