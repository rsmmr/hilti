# $Id$
"""
Misc
~~~~

"""

import llvm.core

from hilti import hlt
from hilti import type
from hilti.constraints import *
from hilti.instruction import *

@hlt.instruction("nop")
class Nop(Instruction):
    """
    Evaluates into nothing.
    """
    def _codegen(self, cg):
        pass

