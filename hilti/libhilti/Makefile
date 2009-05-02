# $Id$
#
# Build libhilti

BC_FROM_LL = call_hilti.bc

BC_FROM_C  = print.bc exceptions.bc utf8proc.bc init.bc memory.bc string.bc \
             int.bc double.bc bool.bc tuple.bc fmt.bc channel.bc enum.bc \
             bytes.bc config.bc schedulers.bc thread_context.bc run.bc

BC = $(BC_FROM_LL) $(BC_FROM_C) 

HEADERS  = hilti.h hilti_intern.h bytes.h config.h thread_context.h exceptions.h
MAIN     = main.bc

GC_LIB     = gc/gc.a 
GC_INCLUDE = gc/include

# NOTE that llvm-ld looks for *.bca with -l, which isn't
# documented in its man page.
LIB_STATIC      = libhilti.bca
LIB_STATIC_MAIN = libhiltimain.bca

CFLAGS=-emit-llvm -g -Wall -I $(GC_INCLUDE) -std=c99 -pthread
LDFLAGS=

LLVMAS_FLAGS=
LLC_FLAGS=

all: gc $(LIB_STATIC) $(LIB_STATIC_MAIN)

clean:
	make -C gc clean
	rm -f $(LIB_STATIC) $(LIB_STATIC_MAIN) $(BC) $(MAIN)

%.bc : %.c $(HEADERS)
	llvm-gcc -c $(CFLAGS) $< -o $@
    
%.bc : %.ll 
	llvm-as -f $(LLVMAS_FLAGS) $< -o $@

# Temporarily disabled
# 
#.PHONY: gc
#gc : 
#	make -C gc
    
$(LIB_STATIC): $(BC)
	rm -f $(LIB_STATIC)
	llvm-ar rcs $(LIB_STATIC) $(BC)
    
$(LIB_STATIC_MAIN): $(BC) $(MAIN)
	rm -f $(LIB_STATIC_MAIN)
	llvm-ar rcs $(LIB_STATIC_MAIN) $(BC) $(MAIN)

    
    
