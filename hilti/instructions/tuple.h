///
/// \type Tuple
///
/// The *tuple* data type represents ordered tuples of other values, which
/// can be of mixed type. Tuple are however statically types and therefore
/// need the individual types to be specified as type parameters, e.g.,
/// ``tuple<int32,string,bool>`` to represent a 3-tuple of ``int32``,
/// ``string``, and ``bool``. Tuple values are enclosed in parentheses with
/// the individual components separated by commas, e.g., ``(42, "foo",
/// True)``. If not explictly initialized, tuples are set to their
/// components' default values initially.

#include "define-instruction.h"

iBeginH(tuple, Equal, "equal")
    iTarget(optype::boolean);
    iOp1(optype::tuple, true);
    iOp2(optype::tuple, true);
iEndH

iBeginH(tuple, Index, "tuple.index")
    iTarget(optype::any)
    iOp1(optype::tuple, true)
    iOp2(optype::integer, true)
iEndH

iBeginH(tuple, Length, "tuple.length")
    iTarget(optype::int64);
    iOp1(optype::tuple, true)
iEndH
