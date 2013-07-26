/// \type Ports
///
/// The *port* data type represents TCP and UDP port numbers.
///
/// \default 0/tcp
///
/// \ctor 80/tcp, 53/udp
///
/// \cproto hlt_port

#include "define-instruction.h"

iBeginH(port, Equal, "equal")
    iTarget(optype::boolean)
    iOp1(optype::port, true);
    iOp2(optype::port, true);
iEndH

iBeginH(port, Protocol, "port.protocol")
    iTarget(optype::enum_)
    iOp1(optype::port, true)
iEndH

