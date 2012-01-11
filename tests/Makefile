#
# Run all the tests.
#

BTEST=`which btest`

all: tests

tests:
	@$(BTEST) -F printer -f diag.log

brief:
	@rm -f test.log
	@$(BTEST) -b -F printer -f test.log
	@echo Output in test.log

hello-world:
	@$(BTEST) -f diag.log misc/hello-world.hlt

