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

    hlt_exception* excpt = 0;

    fprintf(stderr, "Starting in C\n");

    foo_test(&excpt);

    while ( excpt ) {
        assert(excpt->type == &hlt_exception_yield);
        fprintf(stderr, "Back in C and resuming\n");
        foo_test_resume(excpt, &excpt);
    }

    fprintf(stderr, "Done in C\n");
    return 0;
}

