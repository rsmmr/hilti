
#ifndef AST_EXPRESSION_H
#define AST_EXPRESSION_H

#include "node.h"
#include "constant.h"
#include "ctor.h"
#include "mixin.h"

namespace ast {

template<typename AstInfo>
class Expression;

/// Base class for mix-ins that want to override some of an expression's
/// virtual methods. See Expression for documentation.
template<typename AstInfo>
class ExpressionOverrider : public Overrider<typename AstInfo::expression>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::coercer Coercer;
   typedef typename AstInfo::coerced_expression CoercedExpression;
   typedef typename AstInfo::expression Expression;

   // See Expression::canCoerceTo().
   virtual bool _canCoerceTo(shared_ptr<Type> target) const {
       return Coercer().canCoerceTo(this->object()->type(), target);
   };

   // See Expression::coerceTo().
   virtual shared_ptr<Expression> _coerceTo(shared_ptr<Type> target);

   // See Expression::isConstant().
   virtual bool _isConstant() const { return false; }

   // See Expression::type().
   virtual shared_ptr<Type> _type() const { assert(false); }

   // See Expression::render().
   virtual void _render(std::ostream& out) const {
       out << " -> ";
       this->object()->type()->render(out);
   }
};

/// Base class for AST nodes representing expressions.
template<typename AstInfo>
class Expression : public AstInfo::node, public Overridable<ExpressionOverrider<AstInfo>>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::coercer Coercer;
   typedef typename AstInfo::expression AIExpression;
   typedef typename AstInfo::coerced_expression CoercedExpression;
   typedef typename AstInfo::visitor_interface VisitorInterface;

   /// Constructor.
   ///
   /// l: A location associated with the expression.
   Expression(const Location& l=Location::None) : AstInfo::node(l) {}

   /// Associates a scope with this expression. When an expression is
   /// imported from another module, the scope remembers where it is coming
   /// from.
   void setScope(const string& scope) { _scope = scope; }

   /// Returns the scope associated with the expression.  When an expression is
   /// imported from another module, the scope remembers where it is coming
   /// from. It is set with setScope().
   const string& scope() const { return _scope; }

   /// Checks whether the expression can be coerced into a given target type.
   ///
   /// The default implementation asks the Coercer. Derived types can
   /// override the method to implement expression-specific coercion checks.
   virtual bool canCoerceTo(shared_ptr<Type> target) const {
       return this->overrider()->_canCoerceTo(target);
   };

   /// Coerce an expression of one type into one of another type. This method
   /// must only be called in canCoerceTo() indicates that the coercion is
   /// legal. The default implementation returns an instance of a Coerced
   /// expression.
   virtual shared_ptr<AIExpression> coerceTo(shared_ptr<Type> target) {
       return this->overrider()->_coerceTo(target);
   }

   /// Returns true if the constant represents constant value. Semantics of
   /// this methods are not directly linked to a Constant as a value, but
   /// more generally ask for values that are guarenteed to not change. The
   /// default implementation always returns false.
   ///
   /// Returns: True of the expression value is guaranteed to never change.
   virtual bool isConstant() const { return this->overrider()->_isConstant(); }

   /// Returns the type of the expression. Must be overidden by derived
   /// classes.
   virtual shared_ptr<Type> type() const { return this->overrider()->_type(); }

   /// Prints out the type of the expressions.
   void render(std::ostream& out) const /* override */ {
       this->overrider()->_render(out);
   }

   ACCEPT_DISABLED;

private:
   string _scope;
};

template<typename AstInfo>
inline shared_ptr<typename AstInfo::expression> ExpressionOverrider<AstInfo>::_coerceTo(shared_ptr<Type> target)
{
    if ( this->object()->type()->equal(target) )
        return this->object();

    std::cerr << util::fmt("cannot coerce expression of type %s to type %s",
                           this->object()->type()->repr().c_str(),
                           target->repr().c_str()) << std::endl;

    assert(this->object()->canCoerceTo(target));
    auto coerced = new CoercedExpression(this->object(), target, this->object()->location());
    return shared_ptr<typename AstInfo::expression>(coerced);
}

namespace expression {

namespace mixin {

// Short-cut to save some typing.
#define __EXPRESSION_MIXIN ::ast::MixIn<typename AstInfo::expression, typename ::ast::ExpressionOverrider<AstInfo>>

/// A mix-in class to define a list expression. Each element of the list is
/// an expression itself.
template<typename AstInfo>
class List : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::type Type;

   typedef std::list<node_ptr<Expression>> expr_list;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// exprs: The list of expressions.
   List(Expression* target, const expr_list& exprs) : __EXPRESSION_MIXIN(target, this) {
       _exprs = exprs;
       for ( auto i : exprs )
           target->addChild(i);
   }

   /// Returns the evaluation type of the list expression. The type is the
   /// type of the last expression in the list, or null if the list is empty.
   shared_ptr<Type> _type() const /* override */ {
       return _exprs.size() ? (*_exprs.back()).type() : shared_ptr<Type>();
   }

   void _render(std::ostream& out) const /* override */ {
       bool first = false;
       for ( auto e : _exprs ) {
           if ( ! first )
               out << ", ";
           e->render(out);
       }

       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   expr_list _exprs;
};

/// A mix-in class to define a constructor expression.
template<typename AstInfo>
class Ctor : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::ctor AICtor;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// ctor: The construced value.
   Ctor(Expression* target, shared_ptr<AICtor> ctor) : __EXPRESSION_MIXIN(target, this) {
       _ctor = ctor;
       target->addChild(_ctor);
   }

   /// Returns the constructed value.
   shared_ptr<AICtor> ctor() const { return _ctor; }

   shared_ptr<Type> _type() const /* override */ {
       return _ctor->type();
   }

   void _render(std::ostream& out) const /* override */ {
       out << "ctor";
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AICtor> _ctor;
};

/// A mix-in class to define a constant expression.
template<typename AstInfo>
class Constant : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::constant_expression ConstantExpression;
   typedef typename AstInfo::constant AIConstant;
   typedef typename AstInfo::constant_coercer ConstantCoercer;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// constant: The constant the expression is reflecting.
   Constant(Expression* target, shared_ptr<AIConstant> constant) : __EXPRESSION_MIXIN(target, this) {
       _constant = constant;
       target->addChild(_constant);
   }

   /// Returns the expression's constant.
   shared_ptr<AIConstant> constant() const { return _constant; }

   /// Return's the constants type.
   shared_ptr<Type> _type() const /* override */  {
       return _constant->type();
   }

   /// Checks whether the constant can be coerced into a given target type.
   ///
   /// The implementation first trys the ConstantCoercer. If that can't
   /// handle the coercion, it then tries the normal Expression::canCoerceTo().
   ///
   /// target: The target type.
   ///
   /// Returns: True if the constant can be coerced.
   bool _canCoerceTo(shared_ptr<Type> target) const /* override */ {
       if ( this->object()->type()->equal(target) )
           return true;

       if ( ConstantCoercer().canCoerceTo(_constant, target) )
           return true;

       return this->object()->AstInfo::expression::canCoerceTo(target);
   }

   /// Coerces constant into a constant of a given target type.
   ///
   /// The implementation first asks the ConstantCoercer to do the operation.
   /// If that can't, it then forwards to the normal Expression::coerceTo().
   ///
   /// target: The target type.
   ///
   /// Returns: The coerced constant, or null if coercion to the target type
   /// isn't supported.
   shared_ptr<Expression> _coerceTo(shared_ptr<Type> target) /* override */ {
       if ( this->object()->type()->equal(target) )
           return this->object();

       auto coerced = ConstantCoercer().coerceTo(_constant, target);

       if ( coerced ) {
           auto cexpr = new ConstantExpression(coerced, this->object()->location());
           return shared_ptr<ConstantExpression>(cexpr);
       }

       return this->object()->AstInfo::expression::coerceTo(target);
   }

   /// Always returns true. This is a constant expression.
   bool _isConstant() const /* override */ { return true; }

   void _render(std::ostream& out) const /* override */ {
       _constant->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIConstant> _constant;
};

/// A mix-in class to define expression referencing a Variable.
template<typename AstInfo>
class Variable : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::variable AIVariable;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// var: The referenced variable.
   Variable(Expression* target, shared_ptr<AIVariable> var) : __EXPRESSION_MIXIN(target, this) {
       _var = var;
       target->addChild(_var);
   }

   /// Returns the variable the expression is referening.
   shared_ptr<AIVariable> variable() const { return _var; }

   /// Returns the variable's tyoe.
   shared_ptr<Type> _type() const /* override */ { return _var->type(); }

   void _render(std::ostream& out) const /* override */ {
       _var->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIVariable> _var;
};


/// A mix-in class to define an expression referencing a type.
template<typename AstInfo>
class Type : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type AIType;
   typedef typename AstInfo::type_type TypeType;
   typedef typename AstInfo::expression Expression;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// type: The referenced type.
   Type(Expression* target, shared_ptr<AIType> type) : __EXPRESSION_MIXIN(target, this) {
       __type = type;
       target->addChild(__type);
   }

   /// Returns the referenced type.
   const shared_ptr<AIType> typeValue() const { return __type; }

   /// Returns type type::Type. Sic! The type of this expression is not the
   /// type it represents, but a "meta type".
   shared_ptr<AIType> _type() const /* override */ {
       return shared_ptr<AIType>(new TypeType(__type));
   }

   /// Always returns true. Types don't change.
   bool _isConstant() const /* override */ { return true; }

   void _render(std::ostream& out) const /* override */ {
       __type->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIType> __type;
};


/// A mix-in class to define an expression referencing a code block.
template<typename AstInfo>
class Block : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::block_type BlockType;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::block AIBlock;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// block: The referenced block.
   Block(Expression* target, shared_ptr<AIBlock> block) : __EXPRESSION_MIXIN(target, this) {
       _block = block;
       target->addChild(_block);
   }

   /// Returns the referenced block.
   const shared_ptr<AIBlock> block() const { return _block; }

   /// Returns type::Block.
   shared_ptr<Type> _type() const /* override */  {
       return shared_ptr<Type>(new BlockType());
   }

   /// Always returns true. Code doesn't change.
   bool _isConstant() const /* override */ { return true; }

   void _render(std::ostream& out) const /* override */ {
       _block->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIBlock> _block;
};

/// A mix-in class to define an expression referencing a module
template<typename AstInfo>
class Module : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::module_type ModuleType;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::module AIModule;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// module: The referenced Module.
   Module(Expression* target, shared_ptr<AIModule> module) : __EXPRESSION_MIXIN(target, this) {
       _module = module;
       target->addChild(_module);
   }

   /// Returns the referenced module.
   const shared_ptr<AIModule> module() const { return _module; }

   /// Returns type::Module.
   shared_ptr<Type> _type() const /* override */  {
       return shared_ptr<Type>(new ModuleType());
   }

   // Always returns true. Modules don't change.
   bool _isConstant() const /* override */ { return true; }

   void _render(std::ostream& out) const /* override */ {
       _module->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIModule> _module;
};

/// A mix-in class to define an expression referencing a function..
template<typename AstInfo>
class Function : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::function AIFunction;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// func: The referenced function.
   Function(Expression* target, shared_ptr<AIFunction> func) : __EXPRESSION_MIXIN(target, this) {
       _func = func;
       target->addChild(_func);
   }

   /// Returns the refenenced function.
   const shared_ptr<AIFunction> function() const { return _func; }

   /// Returns the tupe of the referenced function.
   shared_ptr<Type> _type() const /* override */  { return _func->type(); }

   /// Always returns true. Functions don't change.
   bool _isConstant() const /* override */ { return true; }

   void _render(std::ostream& out) const /* override */ {
       _func->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIFunction> _func;
};

/// A mix-in class to define an expression referencing an ID. Normally, these
/// epxression will be replaced with whatever the ID is referencing during a
/// later resolving stage.
template<typename AstInfo>
class ID : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::unknown_type UnknownType;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::id AIID;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// id: The referenced ID.
   ID(Expression* target, shared_ptr<AIID> id) : __EXPRESSION_MIXIN(target, this) {
       _id = id;
       target->addChild(_id);
   }

   /// Returns the ID the expression is referencing.
   const shared_ptr<AIID> id() const { return _id; }

   /// Returns type::Unknown. This is because it's expected we'll resolve the
   /// ID eventually with another expression and then know the type.
   shared_ptr<Type> _type() const /* override */  {
       return shared_ptr<Type>(new UnknownType());
   }

   void _render(std::ostream& out) const /* override */ {
       _id->render(out);
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   node_ptr<AIID> _id;
};

/// A mix-in class to define an expression that has been coerced to a
/// different type.
template<typename AstInfo>
class Coerced : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// expr: The original expression.
   ///
   /// dst: The type into which it has been coerced. Note that by creating
   /// this expression, we assume that the coercion into this type is legal.
   Coerced(Expression* target, shared_ptr<Expression> expr, shared_ptr<Type> dst) : __EXPRESSION_MIXIN(target, this) {
       _expr = expr;
       __type = dst;
       target->addChild(_expr);
       target->addChild(dst);
   }

   /// Returns the original expression.
   shared_ptr<Expression> expression() const {
       return _expr;
       }

   /// Returns the type into which the expression has been coerced.
   shared_ptr<Type> _type() const /* override */  {
       return __type;
   }

   void _render(std::ostream& out) const /* override */ {
       out << "cast<";
       __type->render(out);
       out << ">( ";
       _expr->render(out);
       out << " )";
   }

private:
   node_ptr<Expression> _expr;
   node_ptr<Type> __type;
};

/// A mix-in class to define an expression that's interpretation is left to
/// the code generator.
template<typename AstInfo>
class CodeGen : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// type: The type of the expression.
   ///
   /// cookie: A value which's interpretation is left to the code generator.
   CodeGen(Expression* target, shared_ptr<Type> type, void* cookie) : __EXPRESSION_MIXIN(target, this) {
       _cookie = cookie;
       __type = type;
   }

   /// Returns the value passed into Init.
   void* cookie() const { return _cookie; }

   /// Returns the type passed into Init.
   shared_ptr<Type> _type() const /* override */ {
       return __type;
   }

   void _render(std::ostream& out) const /* override */ {
       out << "<CodeGen expr>";
       ExpressionOverrider<AstInfo>::_render(out);
   }

private:
   void* _cookie;
   node_ptr<Type> __type;
};

/// A mix-in class to define an expression referencing a function parameter.
template<typename AstInfo>
class Parameter : public __EXPRESSION_MIXIN, public ExpressionOverrider<AstInfo>
{
public:
   typedef typename AstInfo::type Type;
   typedef typename AstInfo::expression Expression;
   typedef typename AstInfo::parameter AIParameter;

   /// Constructor.
   ///
   /// target: The expression we're mixed in with.
   ///
   /// param: The parameter referenced by the expression.
   Parameter(Expression* target, shared_ptr<AIParameter> param) : __EXPRESSION_MIXIN(target, this) {
       _param = param;
       target->addChild(_param);
   }

   /// Returns the parameter referenced by the expression.
   const shared_ptr<AIParameter> parameter() const { return _param; }

   /// Returns the type of the parameter.
   shared_ptr<Type> _type() const /* override */  { return _param->type(); }

   /// Returns true if the parameter is marked as constant.
   bool _isConstant() const /* override */ { return _param->constant(); }

private:
   node_ptr<AIParameter> _param;
};

}

}

}

#endif
