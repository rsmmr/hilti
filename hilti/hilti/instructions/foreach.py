# $Id$

import hilti.instruction as instruction
import hilti.signature as signature
import hilti.id as id
import hilti.type as type
import hilti.operand as operand
import hilti.util as util
import hilti.block as block

import operators
import flow

class ForEach(instruction.Instruction):
    
    _signature = signature.Signature("for")
    
    def __init__(self, var, container, blocks, location=None):
        super(ForEach, self).__init__(location=location)
        self._var = var
        self._container = container
        self._blocks = blocks
        
    ### Overridden from Node.
    
    def resolve(self, resolver):
        self._container.resolve(resolver)
        
        # Need to add the loop variable to the function's scope here so that
        # validation will not complain about an unknown identifier. 
        iter_type = self._container.type().refType().iterType()
        var = id.Local("%s" % self._var, iter_type.derefType(), location=self.location())
        resolver.currentFunction().scope().add(var)
        
        for b in self._blocks:
            b.resolve(resolver)
                
    def validate(self, vld):
        self._container.validate(vld)

        for b in self._blocks:
            b.validate(vld)
            
        # TODO: Make sure loop var is not yet defined.
        # TODO: Make sure container is iterable.
        
    def output(self, printer):
        printer.output("for %s in " % self._var)
        self._container.output(printer)
        printer.output(" do", nl=True)
        
        printer.push()
        for b in self._blocks:
            b.output(printer)
        printer.pop()

        printer.output("end", nl=True)
                
    def canonify(self, canonifier):
        self._container.canonify(canonifier)

        canonifier.deleteCurrentInstruction()
        
        func = canonifier.currentFunction()
        scope = func.scope()
        
        iter_type = self._container.type().refType().iterType()
        iter = operand.ID(id.Local("__%s_iter" % self._var, iter_type))
        end = operand.ID(id.Local("__%s_end" % self._var, iter_type))
        eq = operand.ID(id.Local("__%s_eq" % self._var, type.Bool()))

        var = scope.lookup(self._var) # Added in resolve().
        assert var 
        var = operand.ID(var)
        
        scope.add(iter.value())
        scope.add(end.value())
        scope.add(eq.value())
        scope.add(var.value())
        
        loop_name = "@__%s_loop" % self._var
        loop = operand.ID(id.Local(loop_name, type.Label()))
        next_name = "@__%s_next" % self._var
        next = operand.ID(id.Local(next_name, type.Label()))
        exit_name = "@__%s_exit" % self._var
        exit = operand.ID(id.Local(exit_name, type.Label()))
        
        bcur = canonifier.currentTransformedBlock()
        bcur.addInstruction(operators.Begin(target=iter, op1=self._container))
        bcur.addInstruction(operators.End(target=end, op1=self._container))
        bcur.addInstruction(flow.Jump(op1=loop))
        
        bloop = block.Block(func, name=loop_name)
        canonifier.addTransformedBlock(bloop)
        bloop.addInstruction(operators.Equal(target=eq, op1=iter, op2=end))
        bloop.addInstruction(flow.IfElse(op1=eq, op2=exit, op3=next))

        bnext = block.Block(func, name=next_name)
        canonifier.addTransformedBlock(bnext)
        
        bnext.addInstruction(operators.Deref(target=var, op1=iter))
        
        # Add loop body here by canonifying it.
        body = None
        for b in self._blocks:
            if not body:
                body = b.name()
            b.canonify(canonifier)

        bcur = canonifier.currentTransformedBlock()
        bcur.addInstruction(operators.Incr(target=iter, op1=iter))
        bcur.addInstruction(flow.Jump(op1=loop))

        if not body:
            body = bcur.name()
        
        bnext.addInstruction(flow.Jump(operand.ID(id.Local(body, type.Label()))))
        
        bexit = block.Block(func, name=exit_name)
        canonifier.addTransformedBlock(bexit)
        # Exit block must be last so that subsequent instructions will be added
        # there. 
        
    def codegen(self, cg):
        # We canonify this instruction away.
        util.internal_error("cannot be reached")
        
        
