///
/// \type Structures
///
/// The ``struct`` data type groups a set of heterogenous, named fields. Each
/// instance tracks which fields have already been set. Fields may optionally
/// provide default values that are returned when it's read but hasn't been
/// set yet.
///
///

#include "define-instruction.h"

iBeginH(struct_, New, "new")
    iTarget(optype::refStruct);
    iOp1(optype::typeStruct, true);
iEndH

iBeginH(struct_, Get, "struct.get")
    iTarget(optype::any);
    iOp1(optype::refStruct, true);
    iOp2(optype::string, true);
iEndH

iBeginH(struct_, GetDefault, "struct.get_default")
    iTarget(optype::any);
    iOp1(optype::refStruct, true);
    iOp2(optype::string, true);
    iOp3(optype::any, false);
iEndH

iBeginH(struct_, IsSet, "struct.is_set")
    iTarget(optype::boolean);
    iOp1(optype::refStruct, true);
    iOp2(optype::string, true);
iEndH

iBeginH(struct_, Set, "struct.set")
    iOp1(optype::refStruct, false);
    iOp2(optype::string, true);
    iOp3(optype::any, false);
iEndH

iBeginH(struct_, Unset, "struct.unset")
    iOp1(optype::refStruct, false);
    iOp2(optype::string, true);
iEndH
