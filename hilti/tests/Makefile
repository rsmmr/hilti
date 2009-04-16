#
#
# Run HILTI tests. 
#

DIRS = misc core integer double string tuple channel libhilti c-api bytes ref struct enum

all: test

test:
	./run-tester $(DIRS)

brief:
	./run-tester -b $(DIRS)

update-baseline:
	./run-tester -U $(DIRS)
