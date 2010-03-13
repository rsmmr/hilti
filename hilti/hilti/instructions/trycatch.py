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
import exception

class TryCatch(instruction.Instruction):
    
    _signature = signature.Signature("try-catch")
    
    def __init__(self, try_blocks, excpt, catch_blocks, location=None):
        super(TryCatch, self).__init__(location=location)
        self._try = try_blocks
        self._catch = catch_blocks
        self._excpt = excpt
        
    ### Overridden from Node.
    
    def resolve(self, resolver):
        self._excpt = self._excpt.resolve(resolver)
        for b in self._try + self._catch:
            b.resolve(resolver)
                
    def validate(self, vld):
        self._excpt.validate(vld)

        for b in self._try + self._catch:
            b.validate(vld)
            
        # TODO: Make sure excpt is an exception type.
        
    def output(self, printer):
        printer.output("try {", nl=True)
        printer.push()
        for b in self._try:
            b.output(printer)
        printer.pop()
        printer.output("}", nl=True)
        
        printer.output("catch ")
        printer.printType(self._excpt)
        printer.output(" {", nl=True)
        printer.push()
        for b in self._catch:
            b.output(printer)
        printer.pop()
        printer.output("}", nl=True)
                
    def canonify(self, canonifier):
        canonifier.deleteCurrentInstruction()
        
        func = canonifier.currentFunction()        
        
        bcur = canonifier.currentTransformedBlock()

        cont_name = canonifier.makeUniqueLabel("cont")
        cont = operand.ID(id.Local(cont_name, type.Label()))

        try_name = self._try[0].name() if self._try else cont_name
        try_ = operand.ID(id.Local(try_name, type.Label()))

        catch_name = self._catch[0].name() if self._catch else cont_name
        catch = operand.ID(id.Local(catch_name, type.Label()))

        bcur.addInstruction(exception.PushHandler(op1=operand.Type(self._excpt), op2=catch, location=self.location()))
        bcur.addInstruction(flow.Jump(try_))
        
        # Add try body by canonifying it.
        for b in self._try:
            b.canonify(canonifier)
        
        bcur = canonifier.currentTransformedBlock()
        bcur.addInstruction(exception.PopHandler(location=self.location()))
        bcur.addInstruction(flow.Jump(cont))

        # Add catch body likewise by canonifiying it.
        for b in self._catch:
            b.canonify(canonifier)
            
        bcur = canonifier.currentTransformedBlock()
        bcur.addInstruction(flow.Jump(cont))
        
        # The block where execution continues after try/catch.
        bcont = block.Block(func, name=cont_name)
        canonifier.addTransformedBlock(bcont)
        
    def codegen(self, cg):
        # We canonify this instruction away.
        util.internal_error("cannot be reached")
        
        
