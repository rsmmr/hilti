# $Id$

import block

import hilti.instructions.debug

class Canonifier(object):
    """Implements the canonification of a module. Canonification is mandatory
    to perform before starting the code generation. 
    """
    def canonify(self, mod, debug=0):
        """Transforms a module into a canonified form suitable for code
        generation. The canonified form ensures a set of properties that simplify
        the code generation process, such as enforcing a fully linked block
        structure and replacing some generic instructions with more specific ones.
        Once a module has been canonified, it can be passed on to ~~codegenModule.
        
        mod: ~~Module - The module to canonify. The module must be well-formed
        as verified by ~~validateModule. It will be changed in place. 
        
        debug: integer - If debug is non-zero, the canonifier may insert additional
        code for helping with debugging at run-time.

        Note: In the future, the canonifier will also be used to implement
        additional 'syntactic sugar' for the HILTI language, so that we can
        writing programs in HILTI more convenient without requiring additional
        complexity for the code generator.
        
        Todo: Throughout the compiler, the ``canonify()`` docstring should
        document the canonfications being done. 
        """
        self._debug = debug
        self._transformed = None
        self._modules = []
        self._functions = []
        self._label_counter = 0
        self._instr = None
        self._debug = debug
        
        mod.canonify(self)
        
        return True

    def debugMode(self):
        """Returns whether we are compiling with debugging support. 
        
        Returns: int - 0 if noto compiling with debugging support, otherwise
        an integer indicatin the debug level.
        """
        return self._debug

    def startModule(self, m):
        """Indicates that code generation starts for a new module.
        
        m: ~~Module - The module.
        """
        self._modules += [m]
        
    def endModule(self):
        """Indicates that canonicalization is finished with the current
        module."""
        self._module = self._modules[:-1]
        
    class _FunctionState:
        def __init__(self, f):
            self.function = f
            self.label_counter = 0
            self.transformed = []
        
    def startFunction(self, f):
        """Indicates that canonicalization starts for a new function.
        
        f: ~~Function - The function.
        """
        self._functions += [Canonifier._FunctionState(f)]
        
    def endFunction(self):
        """Indicates that canonicalization is finished with the current
        function."""
        # Copy the transformed blocks over to the function.
        self._functions = self._functions[:-1]
        
    def currentModule(self):
        """Returns the current module. Only valid while canonification is in
        progress.
        
        Returns: ~~Module - The current module.
        """
        return self._modules[-1] if len(self._modules) else None
    
    def currentFunction(self):
        """Returns the current function. Only valid while canonification is in
        progress.
        
        Returns: ~~Function - The current function.
        """
        return self._functions[-1].function
    
    def currentFunctionName(self):
        """Returns the fully-qualified name of the current function. Only
        valid while canonification is in progress.
        
        Returns: string - The current function's name.
        """
        
        mod = self.currentModule().name() if self.currentModule() else "<no-module>"
        func = self.currentFunction().name() if self.currentFunction() else "<no-func>"
        return "%s::%s" % (mod, func)
    
    def setCurrentInstruction(self, i):
        """Sets the current instruction the canonicalizer is working on. This
        instruction will be returned by ~~currentInstruction until either this
        method is called with a different instruction, or
        ~~deleteCurrentInstruction is called. 
        
        i: ~~Instruction - The instruction.
        
        Note: This is mainly an internal function used by the ~~Block class to
        track instructions.
        """
        self._instr = i
        
    def deleteCurrentInstruction(self):
        """Clears the current instruction, as returned by ~~currentInstruction.
        
        Note: This is mainly an internal function used by the ~~Block class to
        track instructions. It should be used if an instruction replaced
        itself with another one; because then the Block won't reinsert the
        original instruction.
        """
        self._instr = None
    
    def currentInstruction(self):
        """Returns the current instruction as set by ~~setCurrentInstruction.
        
        Returns: ~~Instruction - The current instruction.
        
        Note: This is mainly an internal function used by the ~~Block class to
        track instructions.
        """
        return self._instr
    
    def addTransformedBlock(self, b): 
        """Adds a block to the list of transformed blocks.
        
        b: ~~Block _ The block to append to the list.
        """
        self._functions[-1].transformed += [b]
        
    def transformedBlocks(self):
        """Returns list of transformed blocks.
        
        Returns: list of ~~Block - The list of already transformed blocks.
        """
        return self._functions[-1].transformed

    def splitBlock(self, ins=None, add_flow_dbg=False):
        """Splits the currently processed block at the current position into
        two new blocks. The ~~currentInstructin will be removed. 
        
        ins: ~~Instruction - If given, this instruction is inserted just
        before where the blocks are split; i.e., it will become the last
        instruction of the new block.
        
        add_flow_dbg: bool - If True and we're compiling in debug mode, a
        ``hilti-flow`` debug message will be inserted at the split point (but
        before *ins*) saying that we're leaving the current function.
        """
        self.deleteCurrentInstruction()
        
        if ins:
            current_block = self.currentTransformedBlock()
            
            if add_flow_dbg and self.debugMode():
                dbg = hilti.instructions.debug.message("hilti-flow", "leaving %s" % self.currentFunctionName())
                current_block.addInstruction(dbg)
            
            current_block.addInstruction(ins)
        
        new_block = block.Block(self.currentFunction(), instructions = [], name=self.makeUniqueLabel(), may_remove=True)
        new_block.setNext(current_block.next())
        self.addTransformedBlock(new_block)
        return new_block
    
    def currentTransformedBlock(self):
        """Returns the block currently being transformed. That is, the block
        which has been added to ~~transformedBlocks last. The block can then
        be further extended.
        
        Returns: ~~Block - The block added most recently to the list of
        transformed blocks, or None if the list is still empty.
        """
        try:
            return self._functions[-1].transformed[-1]
        except IndexError:
            return None
        
    def makeUniqueLabel(self, tag=""):
        """Generates a unique label. The label is guarenteed to be unique
        within the function returned by ``currentFunction``.
        
        tag: string - Will be included in the generated label.
        
        Returns: string - The generated label.
        """
        self._functions[-1].label_counter += 1
        
        if tag:
            tag = "-%s" % tag
        
        return "@__l%d%s" % (self._functions[-1].label_counter, tag)



    
    
    
