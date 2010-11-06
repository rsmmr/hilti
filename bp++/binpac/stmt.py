#! $Id$ 
#
# BinPAC++ statements

builtin_id = id

import node
import scope
import type
import id
import binpac.util as util

import hilti.instruction

class Statement(node.Node):
    """Base class for BinPAC++ statements.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, location=None):
        super(Statement, self).__init__(location=location)
    
    # Methods for derived classes to override.

    def execute(self, cg):
        """Generates HILTI code executing the statement.
        
        Must be overidden by derived classes. Derived classes should generally
        not call the parent's implementation. 
        
        cg: ~~Codegen - The current code generator.
        """
        util.internal_error("Statement.execute() not overidden by %s" % self.__class__)

    def __str__(self):
        return "<generic statement>"
    
class Epsilon(Statement):
    """A no-op statement.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, location=None):
        super(Epsilon, self).__init__(location=location)
    
    ### Overidden from Statement.

    def execute(self, cg):
        pass

    def __str__(self):
        return "<empty statement>"
        
class Block(Statement):
    """A block of statements executed in sequence. Such a block has its own
    local scope. 
    
    pscope: ~~Scope - The parent scope for this block.
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, pscope, stmts=None, location=None):
        super(Block, self).__init__(location=location)
        self._stmts = stmts if stmts != None else []
        self._scope = scope.Scope(None, pscope)

    def scope(self):
        """Returns the block's scope.
        
        Returns: ~~Scope - The scope.
        """
        return self._scope

    def statements(self):
        """Returns the block's statements.
        
        Returns: list of ~~Statement - The statements.
        """
        return self._stmts
    
    def addStatement(self, stmt):
        """Adds a statement to the block.
        stmt: ~~Statement - The statement to add.
        """
        self._stmts += [stmt]
    
    ### Overidden from node.Node.

    def resolve(self, resolver):
        Statement.resolve(self, resolver)

        for stmt in self._stmts:
            stmt.resolve(resolver)
    
    def validate(self, vld):
        Statement.validate(self, vld)
        
        for stmt in self._stmts:
            stmt.validate(vld)
            
    def pac(self, printer):
        printer.push()
        printer.output("{", nl=True)
        for stmt in self._stmts:
            stmt.pac(printer)
        printer.output("}", nl=True)
        printer.pop()

    ### Overidden from Statement.

    def execute(self, cg):
        non_const_inits = []
        
        for i in self._scope.IDs():
            if isinstance(i, id.Local):
                if i.expr():
                    if i.expr().isInit():
                        init = i.expr().hiltiInit(cg)
                    else:
                        init = None
                        non_const_inits += [i]
                else:
                    init = i.type().hiltiDefault(cg)
                
                cg.builder().addLocal(i.name(), i.type().hiltiType(cg), init, force=False, reuse=True)
        
        for i in non_const_inits:
            init = i.expr().evaluate(cg)
            cg.builder().assign(cg.builder().idOp(i._internalName()), init)
                
        for stmt in self._stmts:
            stmt.execute(cg)

    def __str__(self):
        return "; ".join([str(s) for s in self._stmts])
        # return "<statement block>"

class UnitHook(Block):
    """A unit hook is a series of statements executed when a certain
    condition occurs, such as unit field being parsed.

    unit: ~~Unit - The unit this hook is part of.
    
    name: string - The name of the hook. For fields, this is the field name;
    for unit hooks, it's the name of that (e.g., ``%init``). 
    
    prio: int - The priority of the statement. If multiple statements are
    defined for the same field, they are executed in order of decreasing
    priority.
    
    stmts: ~~Block - Block with statements and locals for the hook.
    
    debug: bool - If True, this hook will only compiled in if
    the code generator is including debug code, and it will only be executed
    if at run-time, debug mode is enabled (via the C function
    ~~binpac_enable_debug).
    """        
    def __init__(self, unit, name, prio, stmts=None, debug=False, location=None):
        self._unit = unit
        self._name = name
        self._prio = prio
        self._debug = debug
        
        if stmts:
            assert isinstance(stmts, Block)
            
        hscope = scope.Scope(None, unit.scope())
        hscope.addID(id.Parameter("__self", unit))

        super(UnitHook, self).__init__(hscope, location=location)
        
        if stmts:
            self.setStmts(stmts)
                    
    def setStmts(self, stmts):
        """Sets the statements associated with this hook.
        
        stmts: ~~Block - The statement block.
        """
        stmts.scope().setParent(self.scope())
        
        for stmt in stmts.statements():
            self.addStatement(stmt)
            
        for lid in stmts.scope().IDs():
            if isinstance(lid, id.Local):
                self._scope.addID(lid)
                
            
    def unit(self):
        """Returns the unit associated with the hook.
        
        Returns: ~~Unit - The unit.
        """
        return self._unit
            
    def field(self):
        """Returns the name associated with the hook. For fields, this is the
        field name; for unit hooks, it's the name of that (e.g., ``%init``). 

        Returns: ~~string - The name. 
        """
        return self._name

    def priority(self):
        """Returns the priority associated with the hook.  If multiple statements are
        defined for the same field, they are executed in order of decreasing
        priority.
        
        Returns: int - The priority.
        """
        return self._prio

    def debug(self):
        """Returns whether debugging hooks are enabled.
        
        Returns: bool - True if debuggng hooks are enabled. 
        """
        return self._debug

    ### Methods for derived classes to override.
    
    def hiltiName(self, cg):
        """Returns the internal HILTI name for the hook.
        
        Returns: string - The name.
        """
        return cg.hookName(self.unit(), self._name)

    def hiltiFunctionType(self, cg):
        """Returns the internal HILTI type for the hook.
        
        Returns: hilti.type.Hook - The type.
        """
        self = (hilti.id.Parameter("__self", self.unit().hiltiType(cg)), None)
        return hilti.type.Hook([self], hilti.type.Void())
    
    ### Overidden from node.Node.

    def resolve(self, resolver):
        Block.resolve(self, resolver)
        self._unit.resolve(resolver)
        
        for arg in self._unit.args():
            arg.resolve(resolver)
            self._scope.addID(id.UnitParameter(arg.name(), arg.type()))
            
    def validate(self, vld):
        Block.validate(self, vld)
        self._unit.validate(vld)
        
    def execute(self, cg):
        if self.debug() and not cg.debug():
            # Don't compile debug hook.
            return
        
        if self.debug():
            # Add guard code to check whether debug output has been enabled at
            # run-time.
            fbuilder = cg.builder().functionBuilder()
            hook = fbuilder.newBuilder("__debug_hook")
            cont = fbuilder.newBuilder("__debug_cont")
            
            builder = cg.builder()
            dbg = fbuilder.addTmp("__dbg", hilti.type.Bool())
            builder.call(dbg, builder.idOp("BinPAC::debugging_enabled"), builder.tupleOp([]))
            
            cg.builder().if_else(dbg, hook.labelOp(), cont.labelOp())
            
            cg.setBuilder(hook)
            
        Block.execute(self, cg)
        
        if self.debug():
            cg.builder().jump(cont.labelOp())
            cg.setBuilder(cont)

        cg.builder().return_void()
            
class VarHook(UnitHook):
    """A hook associated with a unit variable.
    
    *name*: string - The name of the variable.
    
    See ~~UnitHook for the other parameters.
    """
    def __init__(self, unit, name, prio, stmts=None, debug=False, location=None):
        super(VarHook, self).__init__(unit, name, prio, stmts, debug, location=location)
    
class FieldHook(UnitHook):
    """A hook associated with a unit field.
    
    *field*: ~~Field - The field.
    
    See ~~UnitHook for the other parameters.
    """
    def __init__(self, unit, field, prio, stmts=None, debug=False, location=None):
        super(FieldHook, self).__init__(unit, field.name(), prio, stmts, debug, location)
        self._field = field
        
    def field(self):
        """Returns the field associated with the hook.
        
        Returns: ~~Field - The field, or None if no field is
        associated with the hook.
        """
        return self._field

    def setField(self, field):
        """Associates a unit field with this hook.
        
        field: ~~Field - The field.
        """
        self._field = field    

    def resolve(self, resolver):
        UnitHook.resolve(self, resolver)
        self._field.resolve(resolver)
            
    def validate(self, vld):
        UnitHook.validate(self, vld)
        self._field.validate(vld)

    ### Overidden from UnitHook.
    
    def hiltiName(self, cg):
        return cg.hookName(self.unit(), self._field)

class SubFieldHook(FieldHook):
    """A hook associated with a unit field for which multiple separately
    triggered hooks may exist. 
        
    See ~~FieldHook for parameters. Note that *field* must be of type
    ~~Container. 
    
    Note: We use this for hooks of switch-clauses.
    """
    
    ### Overidden from UnitHook.
    
    def hiltiName(self, cg):
        return cg.hookName(self.unit(), self._field, addl=self._field.caseNumber())
    
class FieldForEachHook(FieldHook):
    def __init__(self, unit, field, prio, stmts=None, debug=False, location=None):
        """A for-each hook associated with a unit field.
    
        See ~~FieldHook for parameters. Note that *field* must be of type
        ~~Container. 
        """
        assert isinstance(field.type(), type.Container)
        super(FieldForEachHook, self).__init__(unit, field, prio, stmts, debug, location)
        self.scope().addID(id.Parameter("__dollardollar", self.itemType()))
        
    def itemType(self):
        """Returns the type of the field's container items.
        
        Returns: ~~Type - The type of items.
        """
        return self._field.type().itemType()

    def resolve(self, resolver):
        super(FieldForEachHook, self).resolve(resolver)

        # It may not have been resolved earlier.
        self.scope().lookupID("__dollardollar").setType(self.itemType())
    
    ### Overidden from UnitHook.

    def hiltiFunctionType(self, cg):
        arg1 = (hilti.id.Parameter("__self", self.unit().hiltiType(cg)), None)
        arg2 = (hilti.id.Parameter("__dollardollar", self.itemType().hiltiType(cg)), None)
        
        return hilti.type.Hook([arg1, arg2], hilti.type.Bool())

    def hiltiName(self, cg):
        return cg.hookName(self.unit(), self.field(), True)
    
class Print(Statement):
    """The ``print`` statement.
    
    exprs: list of ~Expression - The arguments to print.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, exprs, location=None):
        super(Print, self).__init__(location=location)
        self._exprs = exprs
        
    ### Overidden from node.Node.
    
    def resolve(self, resolver):
        Statement.resolve(self, resolver)
        
        for expr in self._exprs:
            expr.resolve(resolver)
    
    def validate(self, vld):
        Statement.validate(self, vld)
        
        for expr in self._exprs:
            expr.validate(vld)
            
            if isinstance(expr.type(), type.Void):
                vld.error(expr, "cannot print void expressions")

    def pac(self, printer):
        printer.output("print ")
        for i in range(len(self._exprs)):
            self._exprs[i].pac(printer)
            if i != len(self._exprs) - 1:
                printer.output(", ")
                
        printer.output(";", nl=True)

    ### Overidden from Statement.

    def execute(self, cg):
        func = cg.builder().idOp("Hilti::print")
        
        if len(self._exprs):
            ops = [expr.simplify().evaluate(cg) for expr in self._exprs]
        else:
            # Print an empty line.
            ops = [cg.builder().constOp("")]
        
        for i in range(len(ops)):
            builder = cg.builder()
            last = (i == len(ops) - 1)
            nl = builder.constOp(last)
            builder.call(None, func, builder.tupleOp([ops[i], nl]))
            if not last:
                builder.call(None, func, builder.tupleOp([builder.constOp(" "), builder.constOp(False)]))
        
    def __str__(self):
        return "<print statement>"
    
class Expression(Statement):
    """A statement evaluating an expression.

    expr: ~~Expression - The expression to evaluate.
    
    location: ~~Location - The location where the statement was defined. 
    """
    def __init__(self, expr, location=None):
        super(Expression, self).__init__(location=location)
        self._expr = expr
        
    ### Overidden from node.Node.
    
    def resolve(self, resolver):
        Statement.resolve(self, resolver)
        self._expr.resolve(resolver)
    
    def validate(self, vld):
        Statement.validate(self, vld)
        self._expr.validate(vld)

    def pac(self, printer):
        self._expr.pac(printer)
        printer.output(";", nl=True)

    ### Overidden from Statement.

    def execute(self, cg):
        self._expr.simplify().evaluate(cg)
        
    def __str__(self):
        return str(self._expr) + ";"

class IfElse(Statement):
    """A statement branching to one of two alternatives based on the value of
    an expression. 

    cond: ~~Expression - The expression to evaluate; must be convertable a
    boolean value.
    
    yes: ~~Stmt - The statement to execute when *cond* is True. 
    
    no: ~~Stmt, or None - The statement to execute when *cond* is False. Can
    be None if in this case, nothing should be done. 
    
    location: ~~Location - The location where the statement was defined. 
    """
    def __init__(self, cond, yes, no=None, location=None):
        super(IfElse, self).__init__(location=location)
        self._cond = cond
        self._yes = yes
        self._no = no
        
    ### Overidden from node.Node.
    
    def resolve(self, resolver):
        Statement.resolve(self, resolver)
        
        self._cond.resolve(resolver)
        self._yes.resolve(resolver)
        if self._no:
            self._no.resolve(resolver)
    
    def validate(self, vld):
        Statement.validate(self, vld)
        
        self._cond.validate(vld)
        self._yes.validate(vld)
        if self._no:
            self._no.validate(vld)
        
        if not self._cond.canCoerceTo(type.Bool()):
            vld.error(self._cond, "expression must be of type bool")

    def pac(self, printer):
        printer.output("if ( ")
        self._cond.pac(printer)
        printer.output(" ) { ", nl=True)
        printer.push()
        self._yes.pac(printer)
        printer.pop()
        printer.output("}", nl=True)

        if self._no:
            printer.output("else { ", nl=True)
            printer.push()
            self._no.pac(printer)
            printer.pop()
            printer.output("}", nl=True)
            

    def execute(self, cg):
        fbuilder = cg.builder().functionBuilder()
        
        yes = fbuilder.newBuilder("__true")
        no = fbuilder.newBuilder("__false") if self._no else None
        cont = fbuilder.newBuilder("__cont")
        
        cond = self._cond.simplify().coerceTo(type.Bool(), cg).evaluate(cg)
        
        cg.builder().if_else(cond, yes.labelOp(), no.labelOp() if no else cont.labelOp())
        
        cg.setBuilder(yes)
        self._yes.execute(cg)
        
        if not cg.builder().terminated():
            cg.builder().jump(cont.labelOp())
        
        if no:
            cg.setBuilder(no)
            self._no.execute(cg)
            
            if not cg.builder().terminated():
                cg.builder().jump(cont.labelOp())
            
        cg.setBuilder(cont)
        
    def __str__(self):
        return "if ( %s ) { %s } else { %s }" % (self._cond, self._yes, self._no)
    
    
class Return(Statement):
    """The ``return`` statement.
    
    expr: ~Expression - The expr to return, or None for void functions.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, expr, location=None, _hook=False):
        super(Return, self).__init__(location=location)
        self._expr = expr
        self._hook = _hook
        
    ### Overidden from node.Node.

    def resolve(self, resolver):
        Statement.resolve(self, resolver)
        
        if self._expr:
            self._expr.resolve(resolver)
    
    def validate(self, vld):
        Statement.validate(self, vld)
        
        if self._expr:
            self._expr.validate(vld)
        
        if self._hook:
            return
            
        if not vld.currentFunction():
            vld.error(self, "return outside of function")
            return
        
        if vld.currentFunction().resultType() == type.Void and self._expr:
            vld.error(expr, "returning value in void function")
            
        if vld.currentFunction().resultType() != type.Void and not self._expr:
            vld.error(expr, "missing return value")

    def pac(self, printer):
        printer.output("return ")
        self._expr.pac(printer)
        printer.output(";", nl=True)

    ### Overidden from Statement.

    def execute(self, cg):
        if self._expr:
            val = self._expr.simplify().evaluate(cg)
            if not self._hook:
                cg.builder().return_result(val)
            else:
                cg.builder().hook_stop(val)
        else:
            cg.builder().return_void()
        
    def __str__(self):
        return "<return statement>"
    
