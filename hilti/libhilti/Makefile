# $Id$
#
# Build libhilti

BC_FROM_LL = call_hilti.bc 

BC_FROM_C  = print.bc exceptions.bc utf8proc.bc init.bc memory.bc string.bc \
             int.bc double.bc bool.bc tuple.bc fmt.bc channel.bc enum.bc \
             bytes.bc config.bc schedulers.bc thread_context.bc run.bc \
             addr.bc port.bc vector.bc list.bc net.bc regexp.bc

BC = $(BC_FROM_LL) $(BC_FROM_C) type-info.bc

HEADERS  = hilti.h hilti_intern.h bytes.h config.h thread_context.h \
           exceptions.h vector.h list.h net.h regexp.h
           
MAIN     = main.bc

GC_LIB     = gc/gc.a 
GC_INCLUDE = gc/include

# NOTE that llvm-ld looks for *.bca with -l, which isn't
# documented in its man page.
LIB_STATIC      = libhilti.bca
LIB_STATIC_MAIN = libhiltimain.bca
LIB_STATIC_C    = libhilti.a

CFLAGS=-emit-llvm -g -Wall -I $(GC_INCLUDE) -std=c99 -pthread
LDFLAGS=

LLVMAS_FLAGS=
LLC_FLAGS=

all: tre $(LIB_STATIC) $(LIB_STATIC_MAIN) $(LIB_STATIC_C)

clean: tre-clean
	make -C gc clean
	rm -f $(LIB_STATIC) $(LIB_STATIC_MAIN) $(BC) $(MAIN)

%.bc : %.c $(HEADERS)
	llvm-gcc -c $(CFLAGS) $< -o $@
    
%.bc : %.ll 
	llvm-as -f $(LLVMAS_FLAGS) $< -o $@

# Generate type information for libhilti.
type-info.bc: type-info.hlt
	hiltic type-info.hlt -l | llvm-as >type-info.bc

.PHONY: tre
tre:
	@if [ ! -e tre/lib/.libs/libtre.a ]; then \
		( cd tre && CC=llvm-gcc CFLAGS= ./configure --disable-approx --disable-agrep --disable-rpath --enable-static --disable-shared && make ) \
	fi

tre-clean:
	-( cd tre && make distclean )
	
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

$(LIB_STATIC_C): $(BC)
	rm -f $(LIB_STATIC)
	# FIXME: run.c call main_run(), which a  C program using
	# libhilti might not provide. Not sure how to deal with this at
	# the moment.
	llvm-link `echo $(BC) | sed 's/run.bc//g'` | llc -f -o $(LIB_STATIC_C).S
	gcc -c $(LIB_STATIC_C).S -o $(LIB_STATIC_C).o
	ar rcs $(LIB_STATIC_C) $(LIB_STATIC_C).o
	rm -f $(LIB_STATIC_C).S $(LIB_STATIC_C).o
    
    
