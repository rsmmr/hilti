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

from hilti.core.instruction import *
from hilti.core.constraints import *
from hilti.instructions.operators import *

@constraint("string")
def _fieldName(ty, op, i):
    if not op or not isinstance(op, ConstOperand):
        return (False, "index must be constant")
    
    if not isinstance(ty, type.String):
        return (False, "index must be string")

    for (id, default) in i.op1().type().refType().Fields():
        if op.value() == id.name():
            return (True, "")
    
    return (False, "%s is not a field name in %s" % (op.value(), instr.op1().type().refType()))

@constraint("any")
def _fieldType(ty, op, i):
    for (id, default) in i.op1().type().refType().Fields():
        if i.op2().value() == id.name():
            if ty == id.type():
                return (True, "")
            else:
                return (False, "type must be %s" % id.type())
    
    assert False

@overload(New, op1=isType(struct), target=referenceOfOp(1))
class New(Operator):
    """Allocates a new instance of the structure given as *op1*. All fields
    will initially be unset. 
    """
    pass

@instruction("struct.get", op1=referenceOf(struct), op2=_fieldName, target=_fieldType)
class Get(Instruction):
    """Returns the field named *op2* in the struct referenced by *op1*. The
    field name must be a constant, and the type of the target must match the
    field's type. If a field is requested that has not been set, its default
    value is returned if it has any defined. If it has not, an
    ``UndefinedValue`` exception is raised. 
    """

@instruction("struct.set", op1=referenceOf(struct), op2=_fieldName, op3=_fieldType)
class Set(Instruction):
    """
    Sets the field named *op2* in the struct referenced by *op1* to the value
    *op3*. The type of the *op3* must match the type of the field.
    """
    
@instruction("struct.unset", op1=referenceOf(struct), op2=_fieldName)
class Unset(Instruction):
    """Unsets the field named *op2* in the struct referenced by *op1*. An
    unset field appears just as if it had never been assigned an value; in
    particular, it will be reset to its default value if has been one assigned. 
    """

@instruction("struct.is_set", op1=referenceOf(struct), op2=_fieldName, target=bool)
class IsSet(Instruction):
    """Returns *True* if the field named *op2* has been set to a value, and
    *False otherwise. If the instruction returns *True*, a subsequent call to
    ~~Get will not raise an exception. 
    """
    

