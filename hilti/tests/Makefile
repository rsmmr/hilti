#
# Run all the tests. 
#

BTEST=../../BTEST/btest

all: tests

tests:
	@$(BTEST)

brief:
	@$(BTEST) -b
    
brief-debug:
	@$(BTEST) -b -d

update-baseline:
	@$(BTEST) -U
    
auto-BTEST:
	@$(BTEST) -b -F printer
    
