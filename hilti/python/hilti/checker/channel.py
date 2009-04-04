# $Id$
#
# Channel checks.

from hilti.instructions import channel
from hilti.core import *
from checker import checker

@checker.when(channel.Write)
def _(self, i):
    if i.op2().type() != i.op1().type().refType().channelType():
        self.error(i, "type mismatch in second operand")

@checker.when(channel.Read)
def _(self, i):
    if i.target().type() != i.op1().type().refType().channelType():
        self.error(i, "type mismatch in target operand")
