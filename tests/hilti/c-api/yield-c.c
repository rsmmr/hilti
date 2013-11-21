/* $Id
 *
 * @TEST-IGNORE
 */

#include <libhilti.h>
#include <assert.h>

#include "yield.hlt.h"

int main()
{
    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    fprintf(stderr, "Starting in C\n");

    foo_test(&excpt, ctx);

    while ( excpt ) {
        assert(excpt->type == &hlt_exception_yield);
        fprintf(stderr, "Back in C and resuming\n");
        foo_test_resume(excpt, &excpt, ctx);
    }

    fprintf(stderr, "Done in C\n");

    return 0;
}

