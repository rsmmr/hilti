# $Id$
#
# Build libhilti

BITCODE  = entry.bc print.bc main.bc
COBJS    = printf.o
MAINOBJS = main.o

LIB_STATIC      = libhilti.a
LIB_STATIC_MAIN = libhiltimain.a

TMP_BC_ASM = .libhilti.bc.S
TMP_BC_OBJ = .libhilti.bc.o

CFLAGS=-g 
LDFLAGS=

LLVMAS_FLAGS=
LLC_FLAGS=

all: $(LIBSTATIC) 

%.o : %.c 
	gcc -c $(CFLAGS) $< -o $@
    
%.bc : %.ll 
	llvm-as $(LLVMAS_FLAGS) $< -o $@
    
$(TMP_BC_OBJ) : $(BITCODE)
	llvm-link $(BITCODE) | llc $(LLCFLAGS) >$(TMP_BC_OBJ)
	gcc -c $(TMP_BC_OBJ) -o $(TMP_BC_OBJ) 
    
$(LIB_STATIC): $(TMP_BC_OBJ) $(COBJS)
	ar rcs $(LIB_STATIC) $(TMP_BC_OBJ) $(COBJS)
    
    
