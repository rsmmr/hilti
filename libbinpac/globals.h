
#ifndef LIBBINPAC_GLOBALS_H
#define LIBBINPAC_GLOBALS_H

#include "libbinpac++.h"

typedef struct {
    hlt_list* parsers;
    hlt_map* mime_types;
    int8_t    debugging;
} __binpac_globals;

extern void __binpac_globals_init();
extern void __binpac_globals_done();
extern __binpac_globals* __binpac_globals_get();
extern void __binpac_globals_set(__binpac_globals* state);

#endif
