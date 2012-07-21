/* $Id
 *
 * @TEST-IGNORE
 */

#include <libhilti.h>
#include <assert.h>

#include "yield-interleaved.hlt.h"

int main()
{
    hlt_init();

    hlt_exception* excpt0 = 0;
    hlt_exception* excpt1 = 0;
    hlt_exception** excpt = 0;

    fprintf(stderr, "Starting first in C\n");
    foo_test(0, &excpt0);

    fprintf(stderr, "Starting second in C\n");
    foo_test(1, &excpt1);

    int i = 1;

    while ( excpt0 || excpt1 ) {

        if ( ! excpt0 )
            excpt = &excpt1;
        else if ( ! excpt1 )
            excpt = &excpt0;
        else excpt = i == 0 ? &excpt0 : &excpt1;

        assert((*excpt)->type == &hlt_exception_yield);
        fprintf(stderr, "Back in C and resuming %d\n", i);
        foo_test_resume(*excpt, excpt);

        i = 1 - i;
    }

    fprintf(stderr, "Done in C\n");

    hlt_done();
    return 0;
}

