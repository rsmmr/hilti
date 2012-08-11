// @TEST-IGNORE

#include <stdio.h>
#include <libhilti.h>

#include "c-interface.hlt.h"

int main()
{
    hlt_init();

    hlt_exception* excpt = 0;

    int i;

    foo_foo1(21, 1, &excpt);
    int32_t n = foo_foo2(21, &excpt);
    printf("C: %d\n", n);

    hlt_string t = hlt_string_from_asciiz("Foo", 0, 0);
    hlt_string s = foo_foo3(t, &excpt);

    printf("C: ");
    for ( i = 0; i < s->len; i++ )
        printf("%c", s->bytes[i]);
    printf("\n");

    GC_DTOR(s, hlt_string);
    GC_DTOR(t, hlt_string);

    hlt_done();

    return 0;
}
