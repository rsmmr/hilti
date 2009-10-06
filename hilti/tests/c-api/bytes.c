/* $Id$

@TEST-EXEC:  hilti-build -v %INPUT -m -o a.out
@TEST-EXEC:  ./a.out >output 2>&1
@TEST-EXEC:  test-diff output

*/

#include <stdio.h>

#include <hilti.h>

void printb(const hlt_bytes* b)
{
    hlt_exception* e = 0;
    hlt_string s = hlt_bytes_to_string(0, &b, 0, &e);
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

const char* myexp(const hlt_exception* e)
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
    
    hlt_exception* e = 0;
    
    hlt_bytes* b = hlt_bytes_new(&e);
    hlt_bytes* b2 = hlt_bytes_new(&e);
 
    printf("len = %d (0) %s\n", hlt_bytes_len(b, &e), myexp(e));
    printf("empty = %d (1) %s\n", hlt_bytes_empty(b, &e), myexp(e));

    hlt_bytes_append_raw(b, (int8_t*)"12345", 5, &e);
    printf("len = %d (5) %s\n", hlt_bytes_len(b, &e), myexp(e));
    printf("empty = %d (0) %s\n", hlt_bytes_empty(b, &e), myexp(e));

    hlt_bytes_append_raw(b, (int8_t*)"12345", 5, &e);
    printf("len = %d (10) %s\n", hlt_bytes_len(b, &e), myexp(e));
    
    hlt_bytes_append_raw(b, large, 2048, &e);
    printf("len = %d (2058) %s\n", hlt_bytes_len(b, &e), myexp(e));

    hlt_bytes_append(b2, b, &e);
    printf("len = %d (2058) %s\n", hlt_bytes_len(b2, &e), myexp(e));
    
    hlt_bytes_append(b2, b2, &e);
    printf("len = %d (exception) %s\n", hlt_bytes_len(b2, &e), myexp(e));

    e = 0;
    hlt_bytes_append(b2, b, &e);
    printf("len = %d (4116) %s\n", hlt_bytes_len(b2, &e), myexp(e));
    
    hlt_bytes_append_raw(b2, (int8_t*)"12345", 5, &e);
    printf("len = %d (4121) %s\n", hlt_bytes_len(b2, &e), myexp(e));
    
    hlt_bytes_append_raw(b2, (int8_t*)0, 0, &e);
    printf("len = %d (4121) %s\n", hlt_bytes_len(b2, &e), myexp(e));

    ///

    printf("-----\n");
    printb(b);
    printf("-----\n");
    
    hlt_bytes_pos p = hlt_bytes_offset(b, 3, &e);
    hlt_bytes_pos p2 = hlt_bytes_offset(b, 7, &e);
    hlt_bytes* sub = hlt_bytes_sub(p, p2, &e);
    printb(sub);
    printf("A len = %d (4) %s\n", hlt_bytes_len(sub, &e), myexp(e));

    printf("-----\n");
    
    sub = hlt_bytes_sub(hlt_bytes_begin(b, &e), p2, &e);
    printb(sub);
    printf("B len = %d (7) %s\n", hlt_bytes_len(sub, &e), myexp(e));
    
    printf("-----\n");
    
    sub = hlt_bytes_sub(p2, hlt_bytes_end(&e), &e);
    printb(sub);
    printf("C len = %d (2051) %s\n", hlt_bytes_len(sub, &e), myexp(e));
    
    printf("-----\n");

    sub = hlt_bytes_sub(hlt_bytes_begin(b, &e), hlt_bytes_end(&e), &e);
    printb(sub);
    printf("D len = %d (%d) %s\n", hlt_bytes_len(sub, &e), hlt_bytes_len(b, &e), myexp(e));

    printf("-----\n");

    p = hlt_bytes_offset(b2, 3, &e);
    p2 = hlt_bytes_offset(b2, 1500, &e);
    sub = hlt_bytes_sub(p, p2, &e);
    printb(sub);
    printf("E len = %d (1497) %s\n", hlt_bytes_len(sub, &e), myexp(e));

    printf("-----\n");
    
    ///
    
    p = hlt_bytes_offset(b, 3, &e);
    p2 = hlt_bytes_offset(b, 7, &e);
    const int8_t* rsub = hlt_bytes_sub_raw(p, p2, &e);
    int d = hlt_bytes_pos_diff(p, p2, &e);
    printc(rsub, d);
    printf("A len = %d (4) %s\n", hlt_bytes_len(sub, &e), myexp(e));

    printf("-----\n");

    rsub = hlt_bytes_sub_raw(hlt_bytes_begin(b, &e), p2, &e);
    d = hlt_bytes_pos_diff(hlt_bytes_begin(b, &e), p2, &e);
    printc(rsub, d);
    printf("B len = %d (7) %s\n", d, myexp(e));
    
    printf("-----\n");
    
    rsub = hlt_bytes_sub_raw(p2, hlt_bytes_end(&e), &e);
    d = hlt_bytes_pos_diff(p2, hlt_bytes_end(&e), &e);
    printc(rsub, d);
    printf("C len = %d (2051) %s\n", d, myexp(e));
    
    printf("-----\n");
    

    rsub = hlt_bytes_sub_raw(hlt_bytes_begin(b, &e), hlt_bytes_end(&e), &e);
    d = hlt_bytes_pos_diff(hlt_bytes_begin(b, &e), hlt_bytes_end(&e), &e);
    printc(rsub, d);
    printf("D len = %d (%d) %s\n", d, hlt_bytes_len(b, &e), myexp(e));

    printf("-----\n");

    p = hlt_bytes_offset(b2, 3, &e);
    p2 = hlt_bytes_offset(b2, 1500, &e);
    rsub = hlt_bytes_sub_raw(p, p2, &e);
    d = hlt_bytes_pos_diff(p, p2, &e);
    printc(rsub, d);
    printf("E len = %d (1497) %s\n", d, myexp(e));

    printf("-----\n");

    ///
    
    e = 0;
    b = hlt_bytes_new(&e);
    int sum2 = 0;
    
    for ( i = 0; i < LEN; i++ )
        hlt_bytes_append_raw(b, large + i, 1, &e);

    p = hlt_bytes_begin(b, &e);
    for ( ; ! hlt_bytes_pos_eq(p, hlt_bytes_end(&e), &e); p = hlt_bytes_pos_incr(p, &e) ) {
        int8_t u = hlt_bytes_pos_deref(p, &e);
        sum2 += u;
    }
    
    printf("sum2 = %d (%d) %s\n", sum2, sum, myexp(e));

    p = hlt_bytes_offset(b, 0, &e);
    p2 = hlt_bytes_offset(b, -1, &e);
    printf("diff = %d (2047) %s\n", hlt_bytes_pos_diff(p, p2, &e), myexp(e));
    
    ///
 
    e = 0;
    b = hlt_bytes_new(&e);
    sum2 = 0;
    
    for ( i = 0; i < 10; i++ )
        hlt_bytes_append_raw(b, large, LEN, &e);

    p = hlt_bytes_begin(b, &e);
    for ( ; ! hlt_bytes_pos_eq(p, hlt_bytes_end(&e), &e); p = hlt_bytes_pos_incr(p, &e) ) {
        int8_t u = hlt_bytes_pos_deref(p, &e);
        sum2 += u;
    }
    
    printf("sum2 = %d (%d) %s\n", sum2, sum * 10, myexp(e));

    p = hlt_bytes_offset(b, 0, &e);
    p2 = hlt_bytes_offset(b, -1, &e);
    printf("diff = %d (%d) %s\n", hlt_bytes_pos_diff(p, p2, &e), LEN * 10 - 1, myexp(e));
        
    ///
    
    p = hlt_bytes_begin(b, &e);
    printf("eq = %d (1) %s\n", hlt_bytes_pos_eq(p, hlt_bytes_begin(b, &e), &e), myexp(e));
    printf("eq = %d (0) %s\n", hlt_bytes_pos_eq(p, hlt_bytes_end(&e), &e), myexp(e));
    p = hlt_bytes_pos_incr(p, &e);
    printf("eq = %d (0) %s\n", hlt_bytes_pos_eq(p, hlt_bytes_begin(b, &e), &e), myexp(e));
    
    p2 = hlt_bytes_end(&e);
    printf("eq = %d (0) %s\n", hlt_bytes_pos_eq(p2, hlt_bytes_begin(b, &e), &e), myexp(e));
    printf("eq = %d (1) %s\n", hlt_bytes_pos_eq(p2, hlt_bytes_end(&e), &e), myexp(e));

    p = hlt_bytes_offset(b, 0, &e);
    printf("eq = %d (1) %s\n", hlt_bytes_pos_eq(p, hlt_bytes_begin(b, &e), &e), myexp(e));

    p = hlt_bytes_offset(b, -1, &e);
    p = hlt_bytes_pos_incr(p, &e);
    printf("eq = %d (1) %s\n", hlt_bytes_pos_eq(p, hlt_bytes_end(&e), &e), myexp(e));

    return 0;
    
}
