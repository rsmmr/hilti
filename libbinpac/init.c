
#include "libbinpac++.h"
#include "libbinpac/globals.h"

void __binpac_init()
{
    if ( ! __binpac_global_state_init() )
        // Already initialized.
        return;

    __binpac_register_pending_parsers();
}

void __binpac_done()
{
    __binpac_global_state_done();
}

void binpac_init()
{
    __binpac_init();
    atexit(__binpac_done);
}
