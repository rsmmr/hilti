
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "stackmap.h"

void __hlt_stackmap_init()
{
#ifdef HAVE_LLVM_STACKMAPS
    __llvm_stackmap* sm = __hlt_llvm_stackmap();

    if ( ! sm->version ) {
        fprintf(stderr, "warning: no stackmap available\n");
        return;
    }

#ifdef DEBUG
    fprintf(stderr, "version %d\n", sm->version);
    fprintf(stderr, "%u functions, %u constants, %u records\n", sm->num_functions,
            sm->num_constants, sm->num_records);

    __llvm_stackmap_function* functions = &sm->functions;

    __llvm_stackmap_constant* constants =
        (__llvm_stackmap_constant*)((char*)functions +
                                    sm->num_functions * sizeof(__llvm_stackmap_function));

    __llvm_stackmap_record* records =
        (__llvm_stackmap_record*)((char*)constants +
                                  sm->num_constants * sizeof(__llvm_stackmap_constant));

    for ( int i = 0; i < sm->num_records; i++ ) {
        __llvm_stackmap_record* r = &records[i];
        fprintf(stderr, "record %d\n", i);
        fprintf(stderr, "  id       = %" PRIu64 "\n", r->id);
        fprintf(stderr, "  offset   = %" PRIu32 "\n", r->offset);
        fprintf(stderr, "  num_locs = %" PRIu16 "\n", r->num_locations);

        __llvm_stackmap_location* locations = &r->locations;

        for ( int j = 0; j < r->num_locations; j++ ) {
            __llvm_stackmap_location* l = &locations[j];

            fprintf(stderr, "  [%d] type   = %" PRIu8 "\n", j, l->type);
            fprintf(stderr, "  [%d] regnum = %" PRIu16 "\n", j, l->dwarf_regnum);
        }
    }
#endif

#endif
}

void __hlt_stackmap_done()
{
#ifdef HAVE_LLVM_STACKMAPS
#endif
}
