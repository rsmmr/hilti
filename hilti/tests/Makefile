#
#
# Run HILTI tests. 
#

DIRS = misc core integer 

all: test

test:
	./run-tester $(DIRS)

update-baseline:
	./run-tester -U $(DIRS)
