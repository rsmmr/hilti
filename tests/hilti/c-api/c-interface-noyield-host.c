// @TEST-IGNORE

#include <stdio.h>
#include <libhilti.h>

#include "c-interface-noyield.hlt.h"

int main()
{
    hlt_init();

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    int i;

    foo_foo1(21, 1, &excpt, ctx);
    int32_t n = foo_foo2(21, &excpt, ctx);
    printf("C: %d\n", n);

    hlt_string t = hlt_string_from_asciiz("Foo", &excpt, ctx);
    hlt_string s = foo_foo3(t, &excpt, ctx);

    printf("C: ");
    for ( i = 0; i < s->len; i++ )
        printf("%c", s->bytes[i]);
    printf("\n");

    return 0;
}
