/* $Id$

@TEST-EXEC:  hilti-build -v %INPUT -m -o a.out
@TEST-EXEC:  ./a.out >output 2>&1
@TEST-EXEC:  btest-diff output

*/

#include <stdio.h>
#include <ctype.h>

#include <libhilti.h>

void printb(const hlt_bytes* b)
{
    hlt_exception* e = 0;
    hlt_string s = hlt_bytes_to_string(0, &b, 0, &e, 0);
    int i;
    for ( i = 0; i < s->len; i++ )
        printf("%c", s->bytes[i]);
    printf("\n");

    GC_DTOR(s, hlt_string);
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
    const int LEN = 2048;
    int i = 0;
    int8_t* large = hlt_malloc(LEN);

    int sum = 0;
    for ( i = 0; i < LEN; i++ ) {
        int8_t u = (int8_t) (i % 256);
        large[i] = u ;
        sum += u;
    }

    hlt_exception* e = 0;

    hlt_bytes* b = hlt_bytes_new(&e, 0);
    hlt_bytes* b2 = hlt_bytes_new(&e, 0);

    printf("len = %ld (0) %s\n", hlt_bytes_len(b, &e, 0), myexp(e));
    printf("empty = %d (1) %s\n", hlt_bytes_empty(b, &e, 0), myexp(e));

    hlt_bytes_append_raw_copy(b, (int8_t*)"12345", 5, &e, 0);
    printf("len = %ld (5) %s\n", hlt_bytes_len(b, &e, 0), myexp(e));
    printf("empty = %d (0) %s\n", hlt_bytes_empty(b, &e, 0), myexp(e));

    hlt_bytes_append_raw_copy(b, (int8_t*)"12345", 5, &e, 0);
    printf("len = %ld (10) %s\n", hlt_bytes_len(b, &e, 0), myexp(e));

    hlt_bytes_append_raw_copy(b, large, 2048, &e, 0);
    printf("len = %ld (2058) %s\n", hlt_bytes_len(b, &e, 0), myexp(e));

    hlt_bytes_append(b2, b, &e, 0);
    printf("len = %ld (2058) %s\n", hlt_bytes_len(b2, &e, 0), myexp(e));

    hlt_bytes_append(b2, b2, &e, 0);
    printf("len = %ld (exception) %s\n", hlt_bytes_len(b2, &e, 0), myexp(e));

    e = 0;
    hlt_bytes_append(b2, b, &e, 0);
    printf("len = %ld (4116) %s\n", hlt_bytes_len(b2, &e, 0), myexp(e));

    hlt_bytes_append_raw_copy(b2, (int8_t*)"12345", 5, &e, 0);
    printf("len = %ld (4121) %s\n", hlt_bytes_len(b2, &e, 0), myexp(e));

    hlt_bytes_append_raw_copy(b2, (int8_t*)0, 0, &e, 0);
    printf("len = %ld (4121) %s\n", hlt_bytes_len(b2, &e, 0), myexp(e));

    ///

    printf("-----\n");
    printb(b);
    printf("-----\n");

    hlt_iterator_bytes p = hlt_bytes_offset(b, 3, &e, 0);
    hlt_iterator_bytes p2 = hlt_bytes_offset(b, 7, &e, 0);
    hlt_bytes* sub = hlt_bytes_sub(p, p2, &e, 0);
    printb(sub);
    printf("A len = %ld (4) %s\n", hlt_bytes_len(sub, &e, 0), myexp(e));

    printf("-----\n");

    sub = hlt_bytes_sub(hlt_bytes_begin(b, &e, 0), p2, &e, 0);
    printb(sub);
    printf("B len = %ld (7) %s\n", hlt_bytes_len(sub, &e, 0), myexp(e));

    printf("-----\n");
    GC_DTOR(sub, hlt_bytes);

    sub = hlt_bytes_sub(p2, hlt_bytes_end(b, &e, 0), &e, 0);
    printb(sub);
    printf("C len = %ld (2051) %s\n", hlt_bytes_len(sub, &e, 0), myexp(e));

    printf("-----\n");
    GC_DTOR(sub, hlt_bytes);

    sub = hlt_bytes_sub(hlt_bytes_begin(b, &e, 0), hlt_bytes_end(b, &e, 0), &e, 0);
    printb(sub);
    printf("D len = %ld (%ld) %s\n", hlt_bytes_len(sub, &e, 0), hlt_bytes_len(b, &e, 0), myexp(e));

    printf("-----\n");
    GC_DTOR(sub, hlt_bytes);

    GC_DTOR(p, hlt_bytes);

    p = hlt_bytes_offset(b2, 3, &e, 0);
    p2 = hlt_bytes_offset(b2, 1500, &e, 0);
    sub = hlt_bytes_sub(p, p2, &e, 0);
    printb(sub);
    printf("E len = %ld (1497) %s\n", hlt_bytes_len(sub, &e, 0), myexp(e));

    printf("-----\n");

    ///

    p = hlt_bytes_offset(b, 3, &e, 0);
    p2 = hlt_bytes_offset(b, 7, &e, 0);
    const int8_t* rsub = hlt_bytes_sub_raw_copy(p, p2, &e, 0);
    hlt_bytes_size d = hlt_iterator_bytes_diff(p, p2, &e, 0);
    printc(rsub, d);
    printf("A len = %ld (4) %s\n", hlt_bytes_len(sub, &e, 0), myexp(e));

    printf("-----\n");

    rsub = hlt_bytes_sub_raw_copy(hlt_bytes_begin(b, &e, 0), p2, &e, 0);
    d = hlt_iterator_bytes_diff(hlt_bytes_begin(b, &e, 0), p2, &e, 0);
    printc(rsub, d);
    printf("B len = %ld (7) %s\n", d, myexp(e));

    printf("-----\n");

    rsub = hlt_bytes_sub_raw_copy(p2, hlt_bytes_end(b, &e, 0), &e, 0);
    d = hlt_iterator_bytes_diff(p2, hlt_bytes_end(b, &e, 0), &e, 0);
    printc(rsub, d);
    printf("C len = %ld (2051) %s\n", d, myexp(e));

    printf("-----\n");


    rsub = hlt_bytes_sub_raw_copy(hlt_bytes_begin(b, &e, 0), hlt_bytes_end(b, &e, 0), &e, 0);
    d = hlt_iterator_bytes_diff(hlt_bytes_begin(b, &e, 0), hlt_bytes_end(b, &e, 0), &e, 0);
    printc(rsub, d);
    printf("D len = %ld (%ld) %s\n", d, hlt_bytes_len(b, &e, 0), myexp(e));

    printf("-----\n");
    GC_DTOR(p, hlt_iterator_bytes);
    GC_DTOR(p2, hlt_iterator_bytes);

    p = hlt_bytes_offset(b2, 3, &e, 0);
    p2 = hlt_bytes_offset(b2, 1500, &e, 0);
    rsub = hlt_bytes_sub_raw_copy(p, p2, &e, 0);
    d = hlt_iterator_bytes_diff(p, p2, &e, 0);
    printc(rsub, d);
    printf("E len = %ld (1497) %s\n", d, myexp(e));

    printf("-----\n");

    GC_DTOR(rsub, hlt_bytes);
    GC_DTOR(p, hlt_iterator_bytes);
    GC_DTOR(p2, hlt_iterator_bytes);

    ///

    e = 0;
    b = hlt_bytes_new(&e, 0);
    int sum2 = 0;

    for ( i = 0; i < LEN; i++ )
        hlt_bytes_append_raw_copy(b, large + i, 1, &e, 0);

    p = hlt_bytes_begin(b, &e, 0);
    for ( ; ! hlt_iterator_bytes_eq(p, hlt_bytes_end(b, &e, 0), &e, 0); p = hlt_iterator_bytes_incr(p, &e, 0) ) {
        int8_t u = hlt_iterator_bytes_deref(p, &e, 0);
        sum2 += u;
    }

    printf("sum2 = %d (%d) %s\n", sum2, sum, myexp(e));

    p = hlt_bytes_offset(b, 0, &e, 0);
    p2 = hlt_bytes_offset(b, -1, &e, 0);
    printf("diff = %ld (2047) %s\n", hlt_iterator_bytes_diff(p, p2, &e, 0), myexp(e));

    GC_DTOR(b, hlt_bytes);

    ///

    e = 0;
    b = hlt_bytes_new(&e, 0);
    sum2 = 0;

    for ( i = 0; i < 10; i++ )
        hlt_bytes_append_raw_copy(b, large, LEN, &e, 0);

    p = hlt_bytes_begin(b, &e, 0);
    for ( ; ! hlt_iterator_bytes_eq(p, hlt_bytes_end(b, &e, 0), &e, 0); p = hlt_iterator_bytes_incr(p, &e, 0) ) {
        int8_t u = hlt_iterator_bytes_deref(p, &e, 0);
        sum2 += u;
    }

    printf("sum2 = %d (%d) %s\n", sum2, sum * 10, myexp(e));

    GC_DTOR(p, hlt_iterator_bytes);

    p = hlt_bytes_offset(b, 0, &e, 0);
    p2 = hlt_bytes_offset(b, -1, &e, 0);
    printf("diff = %ld (%d) %s\n", hlt_iterator_bytes_diff(p, p2, &e, 0), LEN * 10 - 1, myexp(e));

    GC_DTOR(p, hlt_iterator_bytes);
    GC_DTOR(p2, hlt_iterator_bytes);

    ///

    p = hlt_bytes_begin(b, &e, 0);
    printf("eq = %d (1) %s\n", hlt_iterator_bytes_eq(p, hlt_bytes_begin(b, &e, 0), &e, 0), myexp(e));
    printf("eq = %d (0) %s\n", hlt_iterator_bytes_eq(p, hlt_bytes_end(b, &e, 0), &e, 0), myexp(e));
    GC_DTOR(p, hlt_iterator_bytes);

    p = hlt_iterator_bytes_incr(p, &e, 0);
    printf("eq = %d (0) %s\n", hlt_iterator_bytes_eq(p, hlt_bytes_begin(b, &e, 0), &e, 0), myexp(e));

    p2 = hlt_bytes_end(b, &e, 0);
    printf("eq = %d (0) %s\n", hlt_iterator_bytes_eq(p2, hlt_bytes_begin(b, &e, 0), &e, 0), myexp(e));
    printf("eq = %d (1) %s\n", hlt_iterator_bytes_eq(p2, hlt_bytes_end(b, &e, 0), &e, 0), myexp(e));

    GC_DTOR(p, hlt_iterator_bytes);
    GC_DTOR(p2, hlt_iterator_bytes);

    p = hlt_bytes_offset(b, 0, &e, 0);
    printf("eq = %d (1) %s\n", hlt_iterator_bytes_eq(p, hlt_bytes_begin(b, &e, 0), &e, 0), myexp(e));

    p = hlt_bytes_offset(b, -1, &e, 0);
    p = hlt_iterator_bytes_incr(p, &e, 0);
    printf("eq = %d (1) %s\n", hlt_iterator_bytes_eq(p, hlt_bytes_end(b, &e, 0), &e, 0), myexp(e));

    hlt_free(large);

    return 0;

}
