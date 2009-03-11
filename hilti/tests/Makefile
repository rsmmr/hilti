#
#
# Run HILTI tests. 
#

DIRS = misc core integer double string tuple

all: test

test:
	./run-tester $(DIRS)

brief:
	./run-tester -b $(DIRS)

update-baseline:
	./run-tester -U $(DIRS)
