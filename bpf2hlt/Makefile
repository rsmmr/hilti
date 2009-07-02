
all: pktcnt

clean:
	rm -f pktcnt

pktcnt: pktcnt.cc
	g++ `llvm-config --cxxflags --libs --ldflags` `hilti-config --cxxflags --libs --ldflags` pktcnt.cc -o pktcnt -lpcap -lhilti

