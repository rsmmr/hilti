TOOLS := ../tools
HILTI_BUILD := $(TOOLS)/hilti-build

default: http

http: http.pac2
	$(HILTI_BUILD) -o http $(TOOLS)/pac-driver/pac-driver.cc http.pac2

clean:
	-rm -f http
	-rm -fr http.DSYM/

srcclean:
	-rm -f *~
