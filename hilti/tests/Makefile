#
# Run all the tests. 
#

BTEST=../../btest/btest

all: tests

tests:
	@$(BTEST)

brief:
	@$(BTEST) -b
    
brief-debug:
	@rm -f test.log
	@$(BTEST) -b -d -f test.log
	@echo Output in test.log

update-baseline:
	@$(BTEST) -U
    
auto-BTEST:
	@$(BTEST) -b -F printer
    
