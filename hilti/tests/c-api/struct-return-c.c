/* $Id
 * 
 * @TEST-IGNORE
 */
    
#include <stdint.h>

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

struct A fa() {
    struct A q;
    q.x = 1;
    return q; 
}

struct B fb() {
    struct B q;
    q.x = 1;
    return q; 
}

struct C fc() {
    struct C q;
    q.x = 1;
    return q; 
}

struct D fd() {
    struct D q;
    q.x = 1;
    return q; 
}

struct E fe() {
    struct E q;
    q.x = 1;
    q.y = 2;
    return q; 
}

struct F ff() {
    struct F q;
    q.x = 1;
    q.y = 2;
    return q; 
}

struct G fg() {
    struct G q;
    q.x = 1;
    q.y = 2;
    return q; 
}

struct H fh() {
    struct H q;
    q.x = 1;
    q.y = 2;
    return q; 
}

struct I fi() {
    struct I q;
    q.x = 1;
    q.y = 2;
    q.z = 3;
    return q; 
}

struct J fj() {
    struct J q;
    q.x = 1;
    q.y = 2;
    q.z = 3;
    return q; 
}




