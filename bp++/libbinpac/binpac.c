// $Id
//
// Parts of the public BinPAC++ API implemented in C.

#include <hilti.h>

static int8_t debugging = 0;

void binpac_enable_debugging(int8_t enabled)
{
    // Should be (sufficiently) thread-safe.
    debugging = enabled;
}

int8_t binpac_debugging_enabled()
{
    return debugging;
}
