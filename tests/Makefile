#
# Run all the tests.
#

BTEST=`which btest`

all: tests

tests:
	@$(BTEST) -j 5 -F - -F printer -f diag.log

brief:
	@rm -f test.log
	@$(BTEST) -j 5 -b -F - -F printer -f test.log
	@echo Output in test.log

hello-world:
	@$(BTEST) -f diag.log misc/hello-world.hlt

