// @TEST-IGNORE

#include <stdio.h>
#include <hilti_intern.h>

#include "c-interface.h"

int main()
{
    __hlt_exception excpt;
    
    int i;
    
    foo_foo1(21, 1, &excpt);
    int32_t n = foo_foo2(21, &excpt);
    printf("C: %d\n", n);
    
    __hlt_string s = foo_foo3(__hlt_string_from_asciiz("Foo", 0), &excpt);
    printf("C: ");
    for ( i = 0; i < s->len; i++ )
        printf("%c", s->bytes[i]);
    printf("\n");
    
    return 0;
}
