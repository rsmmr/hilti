# $Id$
#
# String checks.

from hilti.instructions import string
from hilti.core import *
from checker import checker

@checker.when(integer.Fmt)
def _(self, i):
    # TODO: Add check for format types here.
    pass


    
    
    


