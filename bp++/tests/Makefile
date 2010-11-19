#
# Run all the tests. 
#

TESTER=../../btest/btest

all: tests

tests:
	@$(TESTER)

brief:
	@$(TESTER) -b
    
brief-debug:
	@rm -f test.log
	@$(TESTER) -b -d -f test.log
	@echo Output in test.log

update-baseline:
	@$(TESTER) -U
