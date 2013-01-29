#
# Run all the tests.
#

BTEST=`which btest`

all: tests

tests:
	-@$(BTEST) -j -f diag.log
	@mv diag.log .diag.log

	-@$(BTEST) -j -f diag.log -a hilti-build binpac
	@mv diag.log .diag.log.hb

	@cat .diag.log .diag.log.hb >diag.log
