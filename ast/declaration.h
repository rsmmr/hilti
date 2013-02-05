
#ifndef AST_DECLARATION_H
#define AST_DECLARATION_H

#include <util/util.h>

#include "node.h"
#include "mixin.h"

namespace ast {

template<typename AstInfo>
class Declaration;

/// Base class for mix-ins that want to override some of a declarations's
/// virtual methods. See Declaration for documentation.
template<typename AstInfo>
class DeclarationOverrider : public Overrider<typename AstInfo::declaration>
{
public:
    typedef typename AstInfo::declaration Declaration;

    virtual bool _isConstant() const { return false; }
};

/// Base class for AST nodes representing ID declarations. A declaration
/// links an ID to an object and at code generation time emits the necessary
/// implementation.
template<typename AstInfo>
class Declaration : public AstInfo::node, public Overridable<DeclarationOverrider<AstInfo>>
{
public:
    typedef typename AstInfo::node Node;
    typedef typename AstInfo::id ID;

    /// A linkage type for the declaration, with interpretation left to the
    /// client application.
    enum Linkage { LOCAL, PRIVATE, EXPORTED, IMPORTED };

    /// Returns the ID being declared.
    shared_ptr<ID> id() const { return _id; }

    /// Sets the declaration's id.
    void setID(shared_ptr<ID> id) { this->removeChild(_id); _id = id; this->addChild(_id); }

    /// Returns the declarartion linkage.
    Linkage linkage() const { return _linkage; }

    /// Sets the declaration's linkage.
    void setLinkage(Linkage l) { _linkage = l; }

    /// Returns true if the declared value is constant.
    virtual bool isConstant() const {
       return this->overrider()->_isConstant();
    }

    string render() /* override */ {
        const char* linkage = "";

        if ( _linkage == EXPORTED )
            linkage = " (exported)";

        return util::fmt("%s%s", _id->name(), linkage);
    }

protected:
    /// Constructor.
    ///
    /// id: The ID being declared.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: Associated location.
    Declaration(shared_ptr<ID> id, Linkage linkage, const Location& l=Location::None) : Node(l) {
       _id = id;
       _linkage = linkage;
       this->addChild(_id);
    }

private:
    node_ptr<ID> _id;
    Linkage _linkage;
};

namespace declaration {

namespace mixin {

// Short-cut to save some typing.
#define __DECLARATION_MIXIN ::ast::MixIn<typename AstInfo::declaration, typename ::ast::DeclarationOverrider<AstInfo>>

/// A mix-in class to define a variable declaration.
template<typename AstInfo>
class Variable : public __DECLARATION_MIXIN, public DeclarationOverrider<AstInfo>
{
public:
    typedef typename AstInfo::declaration Declaration;
    typedef typename AstInfo::id ID;
    typedef typename AstInfo::variable AIVariable;

    // Constructor.
    //
    // id: The ID associated with the variable.
    //
    // var: The declared variable.
    Variable(Declaration* target, shared_ptr<AIVariable> var) : __DECLARATION_MIXIN(target, this) {
       _var = var;
       target->addChild(_var);
    }

    /// Returns the declared variable.
    shared_ptr<AIVariable> variable() const { return _var; }

    /// Variables are never constant.
    bool isConstant() const /* override */ { return false; }

private:
    node_ptr<AIVariable> _var;
};

/// A mix-in class to define a constant declaration.
template<typename AstInfo>
class Constant : public __DECLARATION_MIXIN, public DeclarationOverrider<AstInfo>
{
public:
    typedef typename AstInfo::declaration Declaration;
    typedef typename AstInfo::id ID;
    typedef typename AstInfo::expression AIExpression;

    // Constructor.
    //
    // id: The ID associated with the constant.
    //
    // constant: The constant being declared.
    Constant(Declaration* target, shared_ptr<AIExpression> expr) : __DECLARATION_MIXIN(target, this) {
       _expr = expr;
       target->addChild(_expr);
    }

    /// Returns the declared value.
    shared_ptr<AIExpression> constant() const { return _expr; }

    /// Constants are always constant.
    bool isConstant() const /* override */ { return true; }

private:
    node_ptr<AIExpression> _expr;
};

/// A mix-in class to define a type declaration.
template<typename AstInfo>
class Type : public __DECLARATION_MIXIN, public DeclarationOverrider<AstInfo>
{
public:
    typedef typename AstInfo::declaration Declaration;
    typedef typename AstInfo::id ID;
    typedef typename AstInfo::type AIType;
    typedef typename AstInfo::scope Scope;

    // Constructor.
    //
    // id: The ID associated with the type.
    //
    // type: The type being declared.
    Type(Declaration* target, shared_ptr<AIType> type) : __DECLARATION_MIXIN(target, this) {
       _type = type;
       target->addChild(_type);
    }

    /// Returns the declared type.
    shared_ptr<AIType> type() const { return _type; }

    /// Types are always constant.
    bool isConstant() const /* override */ { return true; }

private:
    node_ptr<AIType> _type;
};

/// A mix-in class to define a function declaration.
template<typename AstInfo>
class Function : public __DECLARATION_MIXIN, public DeclarationOverrider<AstInfo>
{
public:
    typedef typename AstInfo::declaration Declaration;
    typedef typename AstInfo::id ID;
    typedef typename AstInfo::function AIFunction;

    // Constructor.
    //
    // id: The ID associated with the function.
    //
    // func: The declared function.
    Function(Declaration* target, shared_ptr<AIFunction> func) : __DECLARATION_MIXIN(target, this) {
       _func = func;
       target->addChild(_func);
    }

    /// Returns the declared function.
    shared_ptr<AIFunction> function() const { return _func; }

    /// Functions are always constant.
    bool isConstant() const /* override */ { return true; }

private:
    node_ptr<AIFunction> _func;
};

}

}

}

#endif
