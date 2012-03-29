
#ifndef HILTI_CODEGEN_SYSTEM_H
#define HILTI_CODEGEN_SYSTEM_H

#include <machine/endian.h>

#include "common.h"

namespace hilti {

namespace codegen {

namespace system {

enum ByteOrder { LittleEndian, BigEndian };

inline ByteOrder byteOrder()
{
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
    return LittleEndian;
#elif (__BYTE_ORDER == __BIG_ENDIAN)
    return BigEndian;
#else
# error Unknown machine endianness detected.
#endif
}

}

}

}

#endif
