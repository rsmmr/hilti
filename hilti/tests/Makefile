#
#
# Run HILTI tests. 
#

all: test

test:
	./run-tester

brief:
	./run-tester $(TESTER_OPT) -b
    
brief-debug:
	./run-tester $(TESTER_OPT) -b -d

update-baseline:
	./run-tester -U
