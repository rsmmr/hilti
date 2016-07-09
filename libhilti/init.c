
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "globals.h"
#include "linker.h"

void __hlt_init()
{
    if ( ! __hlt_global_state_init() )
        // Already initialized.
        return;

    // Initialize locale.
    if ( ! setlocale(LC_CTYPE, "") ) {
        fprintf(stderr, "libhilti: cannot set locale");
        exit(1);
    }
}

void __hlt_done()
{
    __hlt_global_state_done();
}

void hlt_init()
{
    __hlt_init();
    atexit(__hlt_done);
}
