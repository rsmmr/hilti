/* $Id
 *
 * @TEST-IGNORE
 */

#include <libhilti.h>
#include <assert.h>

#include "yield-with-return.hlt.h"

int main()
{
    hlt_init();

    hlt_exception* excpt = 0;
    int32_t result = 0;

    fprintf(stderr, "Starting in C\n");

    result = foo_test(&excpt);

    while ( excpt ) {
        assert(excpt->type == &hlt_exception_yield);
        fprintf(stderr, "Back in C and resuming\n");
        result = foo_test_resume(excpt, &excpt);
    }

    fprintf(stderr, "Done in C and the result is %d\n", result);

    hlt_done();
    return 0;
}

