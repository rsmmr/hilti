
# Needs GNU make.

BINPAC=../binpac/src/binpac
CXXFLAGS=-I../binpac/lib -I../include

PAC=$(wildcard *.pac)
CCSRCS=$(patsubst %.pac,%_pac.cc,$(PAC))
HDRS=$(patsubst %.cc,%.h,$(CCSRCS))
OBJS=$(patsubst %.cc,%.o,$(CCSRCS))
TARGET=libanalyzers.a

all: $(TARGET)

clean:
	rm -f $(HDRS) $(CCSRCS) $(OBJS) $(TARGET) 

%_pac.o : %.pac
	$(BINPAC) $<
	g++ -c $(CXXFLAGS) `echo $< | sed 's/\.pac/_pac.cc/g'`
	
$(TARGET) : $(OBJS)
	ar rcs $(TARGET) $(OBJS)
