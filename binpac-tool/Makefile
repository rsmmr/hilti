# $Id$
#

CXXFLAGS = -I include -I binpac/lib -I pac
LDFLAGS  = -Lpac -lanalyzers

all: pac bptool

clean:
	$(MAKE) -C pac clean
	rm -f bptool        

.PHONY: pac
pac:
	$(MAKE) -C pac
    
bptool: bptool.cc pac/libanalyzers.a
	g++ -g bptool.cc $(LDFLAGS) $(CXXFLAGS) -o bptool 
    
tar: clean 
	$(MAKE) -C binpac clean
	( cd .. && tar czvf binpac-tool.tgz binpac-tool )
    
    
