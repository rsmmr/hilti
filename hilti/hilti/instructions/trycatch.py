# $Id$

builtin_id = id

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

    def __init__(self, try_blocks, catches, location=None):
        """XXX catches are (excpt, list of blocks) XXXX excpt can be None for
        root XXX"""
        super(TryCatch, self).__init__(location=location)
        self._try = try_blocks
        self._excpts = [(c[0] if c[0] else type.Exception.root()) for c in catches]
        self._catch = [c[1] for c in catches]

    ### Overridden from Node.

    def _resolve(self, resolver):
        self._excpts = [e.resolve(resolver) for e in self._excpts]

        for s in [self._try] + self._catch:
            for b in s:
                b.resolve(resolver)

    def _validate(self, vld):

        for e in self._excpts:
            e.validate(vld)

        for s in [self._try] + self._catch:
            for b in s:
                b.validate(vld)

        # TODO: Make sure excpt is an exception type.

    def output(self, printer):
        printer.output("try {", nl=True)
        printer.push()
        for b in self._try:
            b.output(printer)
        printer.pop()
        printer.output("}", nl=True)

        for (excpt, catch) in zip(self._excpts, self._catch):
            printer.output("catch ")
            if not excpt.isRootType():
                printer.printType(excpt)
            printer.output(" {", nl=True)
            printer.push()
            for b in catch:
                b.output(printer)
                printer.pop()
            printer.output("}", nl=True)

    def _canonify(self, canonifier):
        canonifier.deleteCurrentInstruction()

        func = canonifier.currentFunction()
        bcur = canonifier.currentTransformedBlock()

        cont_name = canonifier.makeUniqueLabel("cont")
        cont = operand.ID(id.Local(cont_name, type.Label()))

        try_name = self._try[0].name() if self._try else cont_name
        try_ = operand.ID(id.Local(try_name, type.Label()))

        # Sort the catch bodies so that if excpt1 is derived from excpt2, the
        # push the latter first.

        def _cmp(x, y):
            x = x[0]
            y = y[0]

            while True:
                if y.isRootType() or isinstance(y, type.Unknown):
                    return 1

                if builtin_id(y.baseClass()) == builtin_id(x):
                    return -1

                y = y.baseClass()

        catches = zip(self._excpts, self._catch)
        catches.sort(cmp=_cmp)

        for (excpt, catch) in catches:
            catch_name = catch[0].name()
            catch_id = operand.ID(id.Local(catch_name, type.Label()))
            et = operand.Type(excpt) if not excpt.isRootType() else None
            bcur.addInstruction(exception.PushHandler(op1=catch_id, op2=et, location=self.location()))

        bcur.addInstruction(flow.Jump(try_))

        # Add try body by canonifying it.
        for b in self._try:
            b.canonify(canonifier)

        bcur = canonifier.currentTransformedBlock()
        for (excpt, catch) in catches:
            bcur.addInstruction(exception.PopHandler(location=self.location()))

        bcur.addInstruction(flow.Jump(cont))

        # Add catch bodies likewise by canonifiying them.
        for (excpt, catch) in catches:
            for b in catch:
                b.canonify(canonifier)

            bcur = canonifier.currentTransformedBlock()
            bcur.addInstruction(flow.Jump(cont))

        # The block where execution continues after try/catch.
        bcont = block.Block(func, name=cont_name)
        canonifier.addTransformedBlock(bcont)

    def _codegen(self, cg):
        # We canonify this instruction away.
        util.internal_error("cannot be reached")


