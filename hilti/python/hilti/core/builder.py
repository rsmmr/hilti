# $Id$

import ast
import instruction
import function

class BlockBuilder: 
    """A class for building sequences of instructions in memory. Each instance
    of a Builder is associated with a ~~Block to which the instruction will be
    added. 
    
    A ~~Builder supports all HILTI instructions as method calls. In general,
    an instruction of the form ``<target> = <part1>.<part2> <op1> <op2>``
    translated into a builder method ``<part1>_<part2>(<target>, <op1>,
    <op2>)`` where ``<target>`` and ``<op1>/<op2>`` are instances of
    ~~instruction.Operand. (Note that these methods are not documented in the
    ~~Builder's method reference.)
    
    block: Block - The block the builder adds instruction to. 
    """
    def __init__(self, block):
        self._block = block
        
    def block(self):
        """Returns the block this builder is adding instructions to.
        
        Returns: Block - The block.
        """
        return self._block
    
    def _setBlock(self, block):
        self._block = block

def _init_builder_instructions():
    # Adds all instructions as methods to the Builder class.
    for (name, ins) in instruction.getInstructions().items():
        
        def _getop(args, sig, op):
            if not sig:
                return (None, args)
            
            if not args and "_optional" in sig.__dict__:
                return (None, args)
        
            if not args:
                raise IndexError(op)
            
            assert isinstance(args[0], instruction.Operand)
            
            return (args[0], args[1:])
        
        def _make_build_function(name, ins, sig):
            def _do_build(self, *args):
                try:
                    (target, args) = _getop(args, sig.target(), "target")
                    (op1, args) = _getop(args, sig.op1(), "operand 1")
                    (op2, args) = _getop(args, sig.op2(), "operand 2")
                    (op3, args) = _getop(args, sig.op3(), "operand 3")
                    
                    if args:
                        raise TypeError("too many arguments for instruction %s" % _do_build.__name__)
                    
                except IndexError, e:
                    raise TypeError("%s missing for instruction %s" % (e, _do_build.__name__))
                
                ins = _do_build._ins(op1=op1, op2=op2, op3=op3, target=target)
                self._block.addInstruction(ins)
                
            _do_build.__name__ = name
            _do_build._ins = ins
            return _do_build
            
        name = name.replace(".", "_")
        BlockBuilder.__dict__[name] = _make_build_function(name, ins, ins._signature)
    
_init_builder_instructions()    
    
class FunctionBuilder(BlockBuilder):
    """A class for building functions. Instructions can be added to the
    function in the same way as provided by ~~Builder for blocks.
    
    module: Module - The module the function will become part of.
    name: string - The name of the function.
    ftype: type.Function - The type of the function.
    cc: function.CallingConvention - The function's calling convention.
    location: Location - The location to be associated with the function.
    """
    def __init__(self, module, name, ftype, cc=function.CallingConvention.HILTI, location=None):
        id = id.ID(name, ftype, id.Role.GLOBAL)
        self._func = Function(ftype, id, cc, location)
        self._module = module
        module.addID(id, self._func)
        self._block = block.Block(self._func)
        self._func.addBlock(self._block)
        super(FunctionBuilder, self).__init__(block)
        
    def startNewBlock(self, name):
        """Starts a new block.
        
        name: string - The label associated with the new block.
        """
        self._block = block.Block(self._func, name=name)
        self._func.addBlock(self._block)
        self._setBlock(self._block)
        
        
    
    
    
    
    
    
    

