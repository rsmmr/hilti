
#ifndef AST_VARIABLE_H
#define AST_VARIABLE_H

#include "node.h"
#include "mixin.h"

namespace ast {

template<typename AstInfo>
class Variable;

/// Base class for mix-ins that want to override some of a variables's
/// virtual methods. See Variable for documentation.
template<typename AstInfo>
class VariableOverrider : public Overrider<typename AstInfo::variable>
{
public:
    typedef typename AstInfo::variable Variable;

    // Empty currently.
};

/// Base class for AST nodes representing variables.
template<typename AstInfo>
class Variable : public AstInfo::node, public Overridable<VariableOverrider<AstInfo>>
{
public:
    typedef typename AstInfo::node Node;
    typedef typename AstInfo::id ID;
    typedef typename AstInfo::type Type;
    typedef typename AstInfo::expression Expression;

    /// Constructor.
    ///
    /// id: The name of the variable. Must be non-scoped.
    ///
    /// type: The type of the variable.
    ///
    /// Expression: An optional initialization expression, or null if none.
    ///
    /// l: Associated location.
    Variable(shared_ptr<ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, const Location& l=Location::None)
       : Node(l) {
           _id = id;
           _type = type;
           _init = init;

           this->addChild(_id);
           this->addChild(_type);
           this->addChild(_init);
       }

    /// Returns the name of the variable.
    shared_ptr<ID> id() const { return _id; };

    /// Returns the type of the variable.
    shared_ptr<Type> type() const { return _type; }

    /// Returns the initialization expression of the variable, or null if
    /// none.
    shared_ptr<Expression> init() const { return _init; }

private:
    node_ptr<ID> _id;
    node_ptr<Type> _type;
    node_ptr<Expression> _init;
};

namespace variable {

namespace mixin {

// Short-cut to save some typing.
#define __VARIABLE_MIXIN ::ast::MixIn<typename AstInfo::variable, typename ::ast::VariableOverrider<AstInfo>>

/// A mix-in class to define a global variable.
template<typename AstInfo>
class Global : public __VARIABLE_MIXIN, public VariableOverrider<AstInfo>
{
public:
    typedef typename AstInfo::variable Variable;

    // Constructor.
    //
    // target: The variable we're mixed in with.
    Global(Variable* target) : __VARIABLE_MIXIN(target, this) {}
};

/// A mix-in class to define a local variable.
template<typename AstInfo>
class Local : public __VARIABLE_MIXIN, public VariableOverrider<AstInfo>
{
public:
    typedef typename AstInfo::variable Variable;

    // Constructor.
    //
    // target: The variable we're mixed in with.
    Local(Variable* target) : __VARIABLE_MIXIN(target, this) {}
};


}

}

}

#endif
