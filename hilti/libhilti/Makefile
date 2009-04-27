# $Id$
#
# Build libhilti

BITCODE  = callhilti.bc 

CHEADERS = hilti.h hilti_intern.h
COBJS    = print.o exceptions.o utf8proc.o init.o memory.o string.o int.o double.o bool.o tuple.o fmt.o channel.o bytes.o enum.o config.o schedulers.o thread_context.o run.o
CMAINOBJ = main.o

GC_LIB     = gc/gc.a 
GC_INCLUDE = gc/include

LIB_STATIC      = libhilti.a
LIB_STATIC_MAIN = libhiltimain.a

TMP_BC_ASM = .libhilti.bc.S
TMP_BC_OBJ = .libhilti.bc.o

CFLAGS=-g -Wall -I $(GC_INCLUDE) -std=c99 
LDFLAGS=

LLVMAS_FLAGS=
LLC_FLAGS=

all: gc $(LIB_STATIC) $(LIB_STATIC_MAIN)

clean:
	make -C gc clean
	rm -f $(LIB_STATIC) $(LIB_STATIC_MAIN) $(TMP_BC_OBJ) $(TMP_BC_ASM) $(BITCODE) $(CMAINOBJ) $(COBJS)

%.o : %.c $(CHEADERS)
	gcc -c $(CFLAGS) $< -o $@
    
%.bc : %.ll 
	llvm-as -f $(LLVMAS_FLAGS) $< -o $@

.PHONY: gc
gc : 
	make -C gc
    
$(TMP_BC_OBJ) : $(BITCODE)
	llvm-link $(BITCODE) | llc $(LLCFLAGS) >$(TMP_BC_ASM)
	gcc -c $(TMP_BC_ASM) -o $(TMP_BC_OBJ) 
    
$(LIB_STATIC): $(TMP_BC_OBJ) $(COBJS)
	cp $(GC_LIB) $(LIB_STATIC)
	ar rcs $(LIB_STATIC) $(TMP_BC_OBJ) $(COBJS)
    
$(LIB_STATIC_MAIN): $(TMP_BC_OBJ) $(COBJS) $(CMAINOBJ)
	cp $(GC_LIB) $(LIB_STATIC_MAIN)
	ar rcs $(LIB_STATIC_MAIN) $(TMP_BC_OBJ) $(COBJS) $(CMAINOBJ) $(GC_LIB)

    
    
