#! $Id$ 
#
# BinPAC++ statements

import ast
import scope
import type
import id
import binpac.util as util

import hilti.instruction

class Statement(ast.Node):
    """Base class for BinPAC++ statements.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, location=None):
        super(Statement, self).__init__(location=location)
    
    # Methods for derived classes to override.

    def execute(self, cg):
        """Generates HILTI code executing the statement.
        
        Must be overidden by derived classes.
        
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
    def __init__(self, pscope, location=None):
        super(Block, self).__init__(location=location)
        self._stmts = []
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
    
    ### Overidden from ast.Node.

    def resolve(self, resolver):
        for stmt in self._stmts:
            stmt.resolve(resolver)
    
    def validate(self, vld):
        for stmt in self._stmts:
            stmt.validate(vld)
            
    def pac(self, printer):
        printer.push()
        printer.output("{", nl=True)
        for stmt in self._stmts:
            stmt.pac(printer)
        printer.output("}", nl=True)
        printer.pop()

    def doSimplify(self):
        self._stmts = [s.simplify() for s in self._stmts]
        return self
        
    ### Overidden from Statement.

    def execute(self, cg):
        for i in self._scope.IDs():
            if isinstance(i, id.Local):
                init = i.expr().hiltiInit(cg) if i.expr() else i.type().hiltiDefault(cg)
                cg.builder().addLocal(i.name(), i.type().hiltiType(cg), init, force=False, reuse=True)
        
        for stmt in self._stmts:
            stmt.execute(cg)

    def __str__(self):
        return "<statement block>"

class UnitHook(Block):
    """A unit hook is a series of statements executed when a certain
    condition occurs, such as unit field being parsed.

    unit: ~~Unit - The unit this hook is part of.
    
    field: ~~Field - The field to which hook is attached, or None if
    not attached a specific field.
    
    prio: int - The priority of the statement. If multiple statements are
    defined for the same field, they are executed in order of decreasing
    priority.
    
    stms: list of ~~Statement - Series of statements to initalize the hook with.
    """
    def __init__(self, unit, field, prio, stmts=[], location=None):
        self._unit = unit
        self._field = field
        self._prio = prio
        
        super(UnitHook, self).__init__(unit.scope(), location=location)
        super(UnitHook, self).scope().addID(id.Local("__hookrc", type.Bool()))
        
        for stmt in stmts:
            self.addStatement(stmt)

    def unit(self):
        """Returns the unit associated with the hook.
        
        Returns: ~~Unit - The unit.
        """
        return self._unit
            
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
    
    def priority(self):
        """Returns the priority associated with the hook.  If multiple statements are
        defined for the same field, they are executed in order of decreasing
        priority.
        
        Returns: int - The priority.
        """
        return self._prio

    
class FieldControlHook(UnitHook):
    """An internal field hook that can exercise control over the parsing. A
    field control is hook is not user-visible but internally created by types
    that define a ``$$`` identifier for use in a field's attribute
    expressions. It is associated with a field by calling ~~setControlHook.
    The hook's ~~scope has the ``$$`` identifier defined. 
    
    field: ~~Field - The field to which hook is attached. 
    
    prio: int - The priority of the statement. If multiple statements are
    defined for the same field, they are executed in order of decreasing
    priority.

    ddtype: ~~Type - The type of the ``$$`` identifier defined by this hook.
    
    stms: list of ~~Statement - Series of statements to initalize the hook with.
    
    """
    def __init__(self, field, prio, ddtype, stmts=[], location=None):
        super(FieldControlHook, self).__init__(field.parent(), field, prio, stmts, location=location)
        self._ddtype = ddtype
        
    def dollarDollarType(self):
        """Returns the type of the hook's ``$$`` identifier.
        
        Returns: ~~Type - The type.
        """
        return self._ddtype

    # Overidden from Block.
    
    def scope(self):
        # Add the $$ identifier. Can't do that in the ctor because types might
        # not be resolved at that time.
        scope = super(UnitHook, self).scope()
        if not scope.lookupID("__dollardollar"):
            scope.addID(id.ID("__dollardollar", self._ddtype))
            
        return scope
    
class Print(Statement):
    """The ``print`` statement.
    
    exprs: list of ~Expression - The arguments to print.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, exprs, location=None):
        super(Print, self).__init__(location=location)
        self._exprs = exprs
        
    ### Overidden from ast.Node.
    
    def resolve(self, resolver):
        for expr in self._exprs:
            expr.resolve(resolver)
    
    def validate(self, vld):
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

    def doSimplify(self):
        self._exprs = [e.simplify() for e in self._exprs]
        return self
        
    ### Overidden from Statement.

    def execute(self, cg):
        func = cg.builder().idOp("Hilti::print")
        
        if len(self._exprs):
            ops = [expr.evaluate(cg) for expr in self._exprs]
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
        
    ### Overidden from ast.Node.
    
    def resolve(self, resolver):
        self._expr.resolve(resolver)
    
    def validate(self, vld):
        self._expr.validate(vld)

    def pac(self, printer):
        self._expr.pac(printer)
        printer.output(";", nl=True)

    def doSimplify(self):
        self._expr = self._expr.simplify()
        return self
        
    ### Overidden from Statement.

    def execute(self, cg):
        self._expr.evaluate(cg)
        
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
        
    ### Overidden from ast.Node.
    
    def resolve(self, resolver):
        self._cond.resolve(resolver)
        self._yes.resolve(resolver)
        if self._no:
            self._no.resolve(resolver)
    
    def validate(self, vld):
        self._cond.validate(vld)
        self._yes.validate(vld)
        if self._no:
            self._no.validate(vld)
        
        # FIXME
        #if not self._cond.canCastTo(type.Bool()):
        #    vld.error(self._cond, "expression must be of type bool")

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
            
    def doSimplify(self):
        self._cond = self._cond.simplify()
        self._yes = self._yes.simplify()
        if self._no:
            self._no.simplify()
            
        if self._cond.isConst():
            if self._cond.constant().value():
                return self._yes
            else:
                if self._no:
                    return self._no
                else:
                    return Epsilon()
                
        return self
        
    ### Overidden from Statement.

    def execute(self, cg):
        fbuilder = cg.builder().functionBuilder()
        
        yes = fbuilder.newBuilder("__true")
        no = fbuilder.newBuilder("__false") if self._no else None
        cont = fbuilder.newBuilder("__cont")
        
        cond = self._cond.evaluate(cg)
        
        cg.builder().if_else(cond, yes.labelOp(), no.labelOp() if no else cont.labelOp())
        
        cg.setBuilder(yes)
        self._yes.execute(cg)
        yes.jump(cont.labelOp())
        
        if no:
            cg.setBuilder(no)
            self._no.execute(cg)
            no.jump(cont.labelOp())
            
        cg.setBuilder(cont)
        
    def __str__(self):
        return "if ( %s ) { %s } else { %s }" % (self._cond, self._yes, self._no)
    
    
class Return(Statement):
    """The ``return`` statement.
    
    expr: ~Expression - The expr to return, or None for void functions.
    
    location: ~~Location - The location where the expression was defined. 
    """
    def __init__(self, expr, location=None):
        super(Return, self).__init__(location=location)
        self._expr = expr
        
    ### Overidden from ast.Node.

    def resolve(self, resolver):
        if self._expr:
            self._expr.resolve(resolver)
    
    def validate(self, vld):
        if self._expr:
            self._expr.validate(vld)
        
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

    def doSimplify(self):
        self._expr = self._expr.simplify()
        return self
        
    ### Overidden from Statement.

    def execute(self, cg):
        if self._expr:
            cg.builder().return_result(self._expr.evaluate(cg))
        else:
            cg.builder().return_void()
        
    def __str__(self):
        return "<return statement>"
