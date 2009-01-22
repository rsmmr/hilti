#
#
# Run HILTI tests. 
#

# Base of HILTI distribution.
HILTI_BASE  = $(shell cd ..  && pwd -P)

TESTER      = $(shell cd ../..  && pwd -P)/tests/bin/tester

# Add HILTI tools to PATH.
PATH        := $(PATH):$(HILTI_BASE)/tools/hiltic:$(HILTI_BASE)/tools/hilti-build
PYTHONPATH  := $(PYTHONPATH):$(HILTI_BASE)/python

# Setup the compiler/assembler/linker options. 
HILTICFLAGS = -I $(HILTI_BASE)/libhilti
LLVMASFLAGS =
LLCFLAGS    =
ASFLAGS     =
LDFLAGS     = -L$(HILTI_BASE)/libhilti

export HILTICFLAGS
export LLVMASFLAGS
export LLCFLAGS
export ASFLAGS
export LDFLAGS

all: test

test:
	$(TESTER) .

update-baseline:
	$(TESTER) -U .
