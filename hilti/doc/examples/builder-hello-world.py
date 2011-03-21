
import sys

import hilti
import hilti.builder

# Modules are the top-level entity, and we start by creating a new
# ModuleBuilder. 
mbuilder = hilti.builder.ModuleBuilder("Main")

# To add the run() function to the module, we need the function's type first.
# The function takes no arguments and doesn't return anything.
ftype = hilti.type.Function([], hilti.type.Void())

# We add the function to the module by creating a corresponding FunctionBuilder.
fbuilder = hilti.builder.FunctionBuilder(mbuilder, "run", ftype)

# Next, we add a body block to the function that will then receive the
# instructions. Each HILTI block is built via its own BlockBuilder. The
# FunctionBuilder has a method that creates a new BlockBuilder, appending a
# block to the function. We just need to give the block a name.
builder = fbuilder.newBuilder("start")

# Now we can start adding instructions to the block, starting with the "call"
# statement. First, we need to create the statement's operands, i.e., the
# function reference and the call's arguments. 

# The "print" function is defined in the "Hilti" module, so we need to import
# that.
if not mbuilder.importModule("hilti"):
	sys.exit(1)

# Create an ID operand referencing the function.
op1 = mbuilder.idOp("Hilti::print")

# Note that function calls require a *tuple* operand for the call's arguments,
# even if it's just one. The BlockBuilder has a method that helps us to create
# one. (The method is actually part of the OperandBuilder, but BlockBuilder is
# derived from that.)
string = builder.constOp("Hello, world!")
op2 = builder.tupleOp([string])

# BlockBuilder provides one method for every instruction that HILTI provides,
# named accordingly. We add instructions simply by calling the corresponding
# method. If an instruction can return a result (like the ``call`` instruction
# might), the method's first argument is always the target operand (which
# may be None if not used); all subsequent method argument correspond to the
# instruction's operands.
builder.call(None, op1, op2)
builder.return_void()

# That's all already, we now just need to finalize the module. This resolves
# all still pending identifier bindings and also validates the module for
# correctness.
errors = mbuilder.finish()

if errors > 0:
	print "%d errors found" % errors
	sys.exit(1)

# Now our program is ready, and we can retrieve the actual HILTI module.
module = mbuilder.module()

# Let's just print it out.
hilti.printModule(module)
