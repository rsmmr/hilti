/* $Id
 *
 * @TEST-IGNORE
 */

#include <libhilti.h>

struct A {
    int8_t x;
};

struct B {
    int16_t x;
};

struct C {
    int32_t x;
};

struct D {
    int64_t x;
};

struct E {
    int8_t x;
    int8_t y;
};

struct F {
    int16_t x;
    int16_t y;
};

struct G {
    int32_t x;
    int32_t y;
};

struct H {
    int64_t x;
    int64_t y;
};

struct I {
    int8_t x;
    int8_t y;
    int8_t z;
};

struct J {
    int32_t x;
    int32_t y;
    int32_t z;
};

struct K {
    int32_t x;
    int64_t y;
};

struct L {
    double d;
    hlt_string s;
};

struct M {
    double d;
    int8_t i;
};

void fa(struct A q) {
    printf("%" PRId8 "\n", q.x);
}

void fb(struct B q) {
    printf("%" PRId16 "\n", q.x);
}

void fc(struct C q) {
    printf("%" PRId32 "\n", q.x);
}

void fd(struct D q) {
    printf("%" PRId64 "\n", q.x);
}

void fe(struct E q) {
    printf("%" PRId8 " %" PRId8 "\n", q.x, q.y);
}

void ff(struct F q) {
    printf("%" PRId16 " %" PRId16 "\n", q.x, q.y);
}

void fg(struct G q) {
    printf("%" PRId32 " %" PRId32 "\n", q.x, q.y);
}

void fh(struct H q) {
    printf("%" PRId64 " %" PRId64 "\n", q.x, q.y);
}

void fi(struct I q) {
    printf("%" PRId8 " %" PRId8 " %" PRId8 "\n", q.x, q.y, q.z);
}

void fj(struct J q) {
    printf("%" PRId16 " %" PRId16 " %" PRId16 "\n", q.x, q.y, q.z);
}

void fk(struct K q) {
    printf("%" PRId32 " %" PRId64 "\n", q.x, q.y);
}

void fl(struct L q) {
    hlt_exception* excpt = 0;
    printf("%f ", q.d);
    hlt_string_print(stdout, q.s, 1, &excpt, hlt_global_execution_context());
}

void fm(struct M q) {
    printf("%f %" PRId8 "\n", q.d, q.i);
}

