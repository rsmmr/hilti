
#include "libspicy.h"
#include "libspicy/globals.h"

void __spicy_init()
{
    if ( ! __spicy_global_state_init() )
        // Already initialized.
        return;

    __spicy_register_pending_parsers();
}

void __spicy_done()
{
    __spicy_global_state_done();
}

void spicy_init()
{
    __spicy_init();
    atexit(__spicy_done);
}
