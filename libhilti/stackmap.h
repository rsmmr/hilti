
#ifndef LIBHILTI_STACKMAP_H
#define LIBHILTI_STACKMAP_H

// #define HAVE_LLVM_STACKMAPS

void __hlt_stackmap_init();
void __hlt_stackmap_done();

// LLVM data structures for representing stackmaps.

typedef struct {
    uint64_t addr;
    uint64_t size;
} __llvm_stackmap_function;

typedef struct {
    uint8_t version;
    uint8_t reserved1;
    uint16_t reserved2;
    uint32_t num_functions;
    uint32_t num_constants;
    uint32_t num_records;
    __llvm_stackmap_function functions;
} __llvm_stackmap;

typedef struct {
    uint64_t large_constant;
} __llvm_stackmap_constant;

typedef struct {
    uint8_t type;
    uint8_t reservered;
    uint16_t dwarf_regnum;
    union {
        int32_t offset;
        int32_t small_constant;
    } arg;
} __llvm_stackmap_location;

typedef struct {
    uint64_t id;
    uint32_t offset;
    uint16_t reservered;
    uint16_t num_locations;
    __llvm_stackmap_location locations;
} __llvm_stackmap_record;

#endif

