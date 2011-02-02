
# The @pactype decorator registers its type in this list.
_types = {}

import node
import codegen
import expr
import grammar
import id
import location
import module
import pgen
import type
import validator
import operator
import scope
import stmt
import printer
import parser
import lexer

# FIXME: We hardcode the constant types we support here. Should do
# that somewhere else.
type._AllowedConstantTypes = (type.Bytes, type.RegExp)
