# $Id$
#
# String checks.

from hilti.instructions import string
from hilti.core import *
from checker import checker

def _checkRefType(i, t):
    if t.refType() != type.Bytes:
        checker.error(i, "operand must be of type bytes")

def _checkEncoding(i, s):
    if not s in "ascii", "utf8":
        checker.error(i, "unknown charset")

@checker.when(string.Encode)
def _(self, i):
    _checkRefType(i, i.op1().type())
    _checkEncoding(i, i.op2().value())

@checker.when(string.Decode)
def _(self, i):
    _checkRefType(i, i.op1().type())
    _checkEncoding(i, i.op2().value())

