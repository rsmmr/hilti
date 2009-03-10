#
#
# Run HILTI tests. 
#

DIRS = misc core integer double string tuple

all: test

test:
	./run-tester $(DIRS)

update-baseline:
	./run-tester -U $(DIRS)
