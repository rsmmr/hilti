#
# Run all the tests.
#

BTEST=`which btest`

all: tests

tests:
	-@$(BTEST) -j -f diag.log
	@mv diag.log .diag.log

	-@$(BTEST) -j -f diag.log -a jit binpac
	@mv diag.log .diag.log.jit

	@cat .diag.log .diag.log.jit >diag.log
