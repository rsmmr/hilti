# $Id$
#

CXXFLAGS = -I include -I binpac/lib -I pac
LDFLAGS  = -Lpac -lanalyzers

all: pac bptool

clean:
	$(MAKE) -C pac clean
	rm bptool        

.PHONY: pac
pac:
	$(MAKE) -C pac
    
bptool: bptool.cc pac/libanalyzers.a
	g++ bptool.cc $(LDFLAGS) $(CXXFLAGS) -o bptool 
