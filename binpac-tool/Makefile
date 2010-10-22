# $Id$
#

BRO=$(HOME)/bro/trunk

CXXFLAGS = -I include -I$(BRO)/aux/binpac/lib -I pac
LDFLAGS  = -Lpac -L$(BRO)/aux/binpac/lib
LIBS = pac/libanalyzers.a $(BRO)/aux/binpac/lib/libbinpac.a -lpcre

all: pac bptool

clean:
	$(MAKE) -C pac clean
	rm -f bptool        

.PHONY: pac
pac:
	$(MAKE) -C pac
    
bptool: bptool.cc pac/libanalyzers.a
	g++ -g bptool.cc $(LDFLAGS) $(CXXFLAGS) -o bptool  $(LIBS)
    
tar: clean 
	$(MAKE) -C binpac clean
	( cd .. && tar czvf binpac-tool.tgz binpac-tool )
    
    
