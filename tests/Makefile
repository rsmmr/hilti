#
# Run all the tests.
#

BTEST=`which btest`

all: tests

tests:
	btest -j -f diag.log

tests-all:
	rm -f diag.log && touch diag.log

	-@$(BTEST) -j -f diag.log.def
	-@$(BTEST) -j -f diag.log.opt  -a optimized
	-@$(BTEST) -j -f diag.log.hb   -a hilti-build spicy
	-@$(BTEST) -j -f diag.log.incr -a incremental spicy

	@cat diag.log.def diag.log.opt diag.log.hb diag.log.incr >diag.log
	@rm  diag.log.def diag.log.opt diag.log.hb diag.log.incr

hello-worlds:
	-@$(BTEST) -f diag.log hilti/misc/hello-world.hlt spicy/misc/hello-world.spicy
