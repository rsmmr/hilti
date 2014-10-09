///
/// \type Structures
///
/// The ``union`` data type groups a set of heterogenous, and optionally
/// namedm fields, of which at any time at most one can be set. Each instance track which field
/// is currently set to allow for type-safe access.

#include "define-instruction.h"

iBeginH(union_, InitField, "union.init")
    iTarget(optype::union_)
    iOp1(optype::typeUnion, true)
    iOp2(optype::string, true)
    iOp3(optype::any, false)
iEndH

iBeginH(union_, InitType, "union.init")
    iTarget(optype::union_)
    iOp1(optype::typeUnion, true)
    iOp2(optype::any, false)
iEndH

iBeginH(union_, GetField, "union.get")
    iTarget(optype::any)
    iOp1(optype::union_, true)
    iOp2(optype::string, true)
iEndH

iBeginH(union_, GetType, "union.get")
    iTarget(optype::any)
    iOp1(optype::union_, true)
iEndH

iBeginH(union_, IsSetField, "union.is_set")
    iTarget(optype::boolean)
    iOp1(optype::union_, true)
    iOp2(optype::string, true)
iEndH

iBeginH(union_, IsSetType, "union.is_set")
    iTarget(optype::boolean)
    iOp1(optype::union_, true)
    iOp2(optype::typeAny, true)
iEndH
