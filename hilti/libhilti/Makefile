# $Id$
#
# Build libhilti

BITCODE  = entry.bc 

COBJS    = printf.o exceptions.o
CMAINOBJ = main.o

LIB_STATIC      = libhilti.a
LIB_STATIC_MAIN = libhiltimain.a

TMP_BC_ASM = .libhilti.bc.S
TMP_BC_OBJ = .libhilti.bc.o

CFLAGS=-g 
LDFLAGS=

LLVMAS_FLAGS=
LLC_FLAGS=

all: $(LIB_STATIC) $(LIB_STATIC_MAIN)

clean:
	rm -f $(LIB_STATIC) $(LIB_STATIC_MAIN) $(TMP_BC_OBJ) $(TMP_BC_ASM) $(BITCODE) $(CMAINOBJ) $(COBJS)

%.o : %.c 
	gcc -c $(CFLAGS) $< -o $@
    
%.bc : %.ll 
	llvm-as -f $(LLVMAS_FLAGS) $< -o $@
    
$(TMP_BC_OBJ) : $(BITCODE)
	llvm-link $(BITCODE) | llc $(LLCFLAGS) >$(TMP_BC_ASM)
	gcc -c $(TMP_BC_ASM) -o $(TMP_BC_OBJ) 
    
$(LIB_STATIC): $(TMP_BC_OBJ) $(COBJS)
	ar rcs $(LIB_STATIC) $(TMP_BC_OBJ) $(COBJS)
    
$(LIB_STATIC_MAIN): $(TMP_BC_OBJ) $(COBJS) $(CMAINOBJ)
	ar rcs $(LIB_STATIC_MAIN) $(TMP_BC_OBJ) $(COBJS) $(CMAINOBJ)
    
    
