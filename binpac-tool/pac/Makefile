
# Needs GNU make.

BRO=$(HOME)/bro/trunk

BINPAC=$(BRO)/aux/binpac/src/binpac
CXXFLAGS=-I$(BRO)/aux/binpac/lib -I../include

#PAC=$(wildcard *.pac)
PAC=binpac.pac binpac-lib.pac test.pac http.pac
CCSRCS=$(patsubst %.pac,%_pac.cc,$(PAC))
HDRS=$(patsubst %.cc,%.h,$(CCSRCS))
# OBJS=$(patsubst %.cc,%.o,$(CCSRCS)) megad_obfuscation_function.o
OBJS=$(patsubst %.cc,%.o,$(CCSRCS))
TARGET=libanalyzers.a

all: $(TARGET)

clean:
	rm -f $(HDRS) $(CCSRCS) $(OBJS) $(TARGET) 

%_pac.o : %.pac
	$(BINPAC) $<
	g++ -g -c $(CXXFLAGS) `echo $< | sed 's/\.pac/_pac.cc/g'`
	
$(TARGET) : $(OBJS)
	ar rcs $(TARGET) $(OBJS)
