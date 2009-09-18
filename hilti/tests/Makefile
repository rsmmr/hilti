#
#
# Run HILTI tests. 
#

DIRS = misc core integer double string tuple channel libhilti c-api \
       bytes ref struct enum threads addr port overlay bool net \
       vector list debug
       
# regexp tests are known to fail.       

all: test

test:
	./run-tester $(DIRS)

brief:
	./run-tester $(TESTER_OPT) -b $(DIRS)

update-baseline:
	./run-tester -U $(DIRS)
