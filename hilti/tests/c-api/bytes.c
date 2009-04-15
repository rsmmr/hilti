/* $Id$

@TEST-EXEC:  ${CC} %INPUT -lhilti -o a.out 
@TEST-EXEC:  ./a.out >output 2>&1
@TEST-EXEC:  test-diff output

*/

#include <stdio.h>

#include <hilti_intern.h>

void printb(const __hlt_bytes* b)
{
    __hlt_exception e = 0;
    const __hlt_string* s = __hlt_bytes_to_string(0, &b, 0, &e);
    int i;
    for ( i = 0; i < s->len; i++ )
        printf("%c", s->bytes[i]);
    printf("\n");
}

void printc(const int8_t* c, int len)
{
    int i = 0;
    for ( i = 0; i < len; i++ ) {
        unsigned char d = (unsigned char) c[i];
        if ( isprint(d) ) 
            printf("%c", d);
        else 
            printf("\\x%02x", d);
    }
    printf("\n");
}

const char* exp(const __hlt_exception e)
{
    if ( e )
        return "excepetion";
    else
        return "no-exception";
}

int main()
{
    const LEN = 2048;
    int i = 0;
    int8_t* large = malloc(LEN);
    
    int sum = 0;
    for ( i = 0; i < LEN; i++ ) {
        int8_t u = (int8_t) (i % 256);
        large[i] = u ;
        sum += u;
    }
    
    __hlt_exception e = 0;
    
    __hlt_bytes* b = __hlt_bytes_new(&e);
    __hlt_bytes* b2 = __hlt_bytes_new(&e);
 
    printf("len = %d (0) %s\n", __hlt_bytes_len(b, &e), exp(e));
    printf("empty = %d (1) %s\n", __hlt_bytes_empty(b, &e)), e;

    __hlt_bytes_append_raw(b, (int8_t*)"12345", 5, &e);
    printf("len = %d (5) %s\n", __hlt_bytes_len(b, &e), exp(e));
    printf("empty = %d (0) %s\n", __hlt_bytes_empty(b, &e), exp(e));

    __hlt_bytes_append_raw(b, (int8_t*)"12345", 5, &e);
    printf("len = %d (10) %s\n", __hlt_bytes_len(b, &e), exp(e));
    
    __hlt_bytes_append_raw(b, large, 2048, &e);
    printf("len = %d (2058) %s\n", __hlt_bytes_len(b, &e), exp(e));

    __hlt_bytes_append(b2, b, &e);
    printf("len = %d (2058) %s\n", __hlt_bytes_len(b2, &e), exp(e));
    
    __hlt_bytes_append(b2, b2, &e);
    printf("len = %d (exception) %s\n", __hlt_bytes_len(b2, &e), exp(e));

    e = 0;
    __hlt_bytes_append(b2, b, &e);
    printf("len = %d (4116) %s\n", __hlt_bytes_len(b2, &e), exp(e));
    
    __hlt_bytes_append_raw(b2, (int8_t*)"12345", 5, &e);
    printf("len = %d (4121) %s\n", __hlt_bytes_len(b2, &e), exp(e));
    
    __hlt_bytes_append_raw(b2, (int8_t*)0, 0, &e);
    printf("len = %d (4121) %s\n", __hlt_bytes_len(b2, &e), exp(e));

    ///

    printf("-----\n");
    printb(b);
    printf("-----\n");
    
    __hlt_bytes_pos* p = __hlt_bytes_offset(b, 3, &e);
    const __hlt_bytes_pos* p2 = __hlt_bytes_offset(b, 7, &e);
    __hlt_bytes* sub = __hlt_bytes_sub(p, p2, &e);
    printb(sub);
    printf("A len = %d (4) %s\n", __hlt_bytes_len(sub, &e), exp(e));

    printf("-----\n");
    
    sub = __hlt_bytes_sub(__hlt_bytes_begin(b, &e), p2, &e);
    printb(sub);
    printf("B len = %d (7) %s\n", __hlt_bytes_len(sub, &e), exp(e));
    
    printf("-----\n");
    
    sub = __hlt_bytes_sub(p2, __hlt_bytes_end(b, &e), &e);
    printb(sub);
    printf("C len = %d (2051) %s\n", __hlt_bytes_len(sub, &e), exp(e));
    
    printf("-----\n");

    sub = __hlt_bytes_sub(__hlt_bytes_begin(b, &e), __hlt_bytes_end(b, &e), &e);
    printb(sub);
    printf("D len = %d (%d) %s\n", __hlt_bytes_len(sub, &e), __hlt_bytes_len(b, &e), exp(e));

    printf("-----\n");

    p = __hlt_bytes_offset(b2, 3, &e);
    p2 = __hlt_bytes_offset(b2, 1500, &e);
    sub = __hlt_bytes_sub(p, p2, &e);
    printb(sub);
    printf("E len = %d (1497) %s\n", __hlt_bytes_len(sub, &e), exp(e));

    printf("-----\n");
    
    ///
    
    p = __hlt_bytes_offset(b, 3, &e);
    p2 = __hlt_bytes_offset(b, 7, &e);
    const int8_t* rsub = __hlt_bytes_sub_raw(p, p2, &e);
    int d = __hlt_bytes_pos_diff(p, p2, &e);
    printc(rsub, d);
    printf("A len = %d (4) %s\n", __hlt_bytes_len(sub, &e), exp(e));

    printf("-----\n");

    rsub = __hlt_bytes_sub_raw(__hlt_bytes_begin(b, &e), p2, &e);
    d = __hlt_bytes_pos_diff(__hlt_bytes_begin(b, &e), p2, &e);
    printc(rsub, d);
    printf("B len = %d (7) %s\n", d, exp(e));
    
    printf("-----\n");
    
    rsub = __hlt_bytes_sub_raw(p2, __hlt_bytes_end(b, &e), &e);
    d = __hlt_bytes_pos_diff(p2, __hlt_bytes_end(b, &e), &e);
    printc(rsub, d);
    printf("C len = %d (2051) %s\n", d, exp(e));
    
    printf("-----\n");
    

    rsub = __hlt_bytes_sub_raw(__hlt_bytes_begin(b, &e), __hlt_bytes_end(b, &e), &e);
    d = __hlt_bytes_pos_diff(__hlt_bytes_begin(b, &e), __hlt_bytes_end(b, &e), &e);
    printc(rsub, d);
    printf("D len = %d (%d) %s\n", d, __hlt_bytes_len(b, &e), exp(e));

    printf("-----\n");

    p = __hlt_bytes_offset(b2, 3, &e);
    p2 = __hlt_bytes_offset(b2, 1500, &e);
    rsub = __hlt_bytes_sub_raw(p, p2, &e);
    d = __hlt_bytes_pos_diff(p, p2, &e);
    printc(rsub, d);
    printf("E len = %d (1497) %s\n", d, exp(e));

    printf("-----\n");

    ///
    
    e = 0;
    b = __hlt_bytes_new(&e);
    int sum2 = 0;
    
    for ( i = 0; i < LEN; i++ )
        __hlt_bytes_append_raw(b, large + i, 1, &e);

    p = __hlt_bytes_begin(b, &e);
    for ( ; ! __hlt_bytes_pos_eq(p, __hlt_bytes_end(b, &e), &e); __hlt_bytes_pos_incr(p, &e) ) {
        int8_t u = __hlt_bytes_pos_deref(p, &e);
        sum2 += u;
    }
    
    printf("sum2 = %d (%d) %s\n", sum2, sum, exp(e));

    p = __hlt_bytes_offset(b, 0, &e);
    p2 = __hlt_bytes_offset(b, -1, &e);
    printf("diff = %d (2047) %s\n", __hlt_bytes_pos_diff(p, p2, &e), exp(e));
    
    ///
 
    e = 0;
    b = __hlt_bytes_new(&e);
    sum2 = 0;
    
    for ( i = 0; i < 10; i++ )
        __hlt_bytes_append_raw(b, large, LEN, &e);

    p = __hlt_bytes_begin(b, &e);
    for ( ; ! __hlt_bytes_pos_eq(p, __hlt_bytes_end(b, &e), &e); __hlt_bytes_pos_incr(p, &e) ) {
        int8_t u = __hlt_bytes_pos_deref(p, &e);
        sum2 += u;
    }
    
    printf("sum2 = %d (%d) %s\n", sum2, sum * 10, exp(e));

    p = __hlt_bytes_offset(b, 0, &e);
    p2 = __hlt_bytes_offset(b, -1, &e);
    printf("diff = %d (%d) %s\n", __hlt_bytes_pos_diff(p, p2, &e), LEN * 10 - 1, exp(e));
        
    ///
    
    p = __hlt_bytes_begin(b, &e);
    printf("eq = %d (1) %s\n", __hlt_bytes_pos_eq(p, __hlt_bytes_begin(b, &e), &e), exp(e));
    printf("eq = %d (0) %s\n", __hlt_bytes_pos_eq(p, __hlt_bytes_end(b, &e), &e), exp(e));
    __hlt_bytes_pos_incr(p, &e);
    printf("eq = %d (0) %s\n", __hlt_bytes_pos_eq(p, __hlt_bytes_begin(b, &e), &e), exp(e));
    
    p2 = __hlt_bytes_end(b, &e);
    printf("eq = %d (0) %s\n", __hlt_bytes_pos_eq(p2, __hlt_bytes_begin(b, &e), &e), exp(e));
    printf("eq = %d (1) %s\n", __hlt_bytes_pos_eq(p2, __hlt_bytes_end(b, &e), &e), exp(e));

    p = __hlt_bytes_offset(b, 0, &e);
    printf("eq = %d (1) %s\n", __hlt_bytes_pos_eq(p, __hlt_bytes_begin(b, &e), &e), exp(e));

    p = __hlt_bytes_offset(b, -1, &e);
    __hlt_bytes_pos_incr(p, &e);
    printf("eq = %d (1) %s\n", __hlt_bytes_pos_eq(p, __hlt_bytes_end(b, &e), &e), exp(e));

    return 0;
    
}
