// @TEST-IGNORE

#include <stdio.h>
#include <libhilti.h>

#include "c-interface.hlt.h"

int main()
{
    hlt_init();

    hlt_exception* excpt;

    int i;

    foo_foo1(21, 1, &excpt);
    int32_t n = foo_foo2(21, &excpt);
    printf("C: %d\n", n);

    hlt_string s = foo_foo3(hlt_string_from_asciiz("Foo", 0, 0), &excpt);
    printf("C: ");
    for ( i = 0; i < s->len; i++ )
        printf("%c", s->bytes[i]);
    printf("\n");

    return 0;
}
