
#ifndef SPICY_DECLARATION_H
#define SPICY_DECLARATION_H

#include <ast/declaration.h>
#include <ast/visitor.h>

#include "common.h"

namespace spicy {

/// Base class for AST declaration nodes.
class Declaration : public ast::Declaration<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared object.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: An associated location.
    Declaration(shared_ptr<spicy::ID> id, Linkage linkage, const Location& l = Location::None);

    /// Returns a readable one-line representation of the declaration.
    string render() override;

    ACCEPT_VISITOR_ROOT();
};

namespace declaration {

/// AST node for declaring a variable.
class Variable : public spicy::Declaration, public ast::declaration::mixin::Variable<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared variable.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// var: The declared variable.
    ///
    /// l: An associated location.
    Variable(shared_ptr<spicy::ID> id, Linkage linkage, shared_ptr<spicy::Variable> var,
             const Location& l = Location::None);

    ACCEPT_VISITOR(spicy::Declaration);
};

/// AST node for declaring a constant value.
class Constant : public spicy::Declaration, public ast::declaration::mixin::Constant<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared constant.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// expr: The declared value.
    ///
    /// l: An associated location.
    Constant(shared_ptr<spicy::ID> id, Linkage linkage, shared_ptr<spicy::Expression> value,
             const Location& l = Location::None);

    ACCEPT_VISITOR(spicy::Declaration);
};

/// AST node for declaring a type.
class Type : public spicy::Declaration, public ast::declaration::mixin::Type<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: The name of declared type.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// type: The declared type.
    ///
    /// l: An associated location.
    Type(shared_ptr<spicy::ID> id, Linkage linkage, shared_ptr<spicy::Type> type,
         const Location& l = Location::None);

    ACCEPT_VISITOR(spicy::Declaration);
};

/// AST node for declaring a function.
class Function : public spicy::Declaration, public ast::declaration::mixin::Function<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// func: The declared function.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: An associated location.
    Function(shared_ptr<spicy::Function> func, Linkage linkage,
             const Location& l = Location::None);

    ACCEPT_VISITOR(spicy::Declaration);
};

/// AST node for declaring a hook.
class Hook : public Declaration {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// hook: The declared hook.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: An associated location.
    Hook(shared_ptr<spicy::ID> id, shared_ptr<spicy::Hook> hook,
         const Location& l = Location::None);

    /// Returns the declared hook.
    shared_ptr<spicy::Hook> hook() const;

    ACCEPT_VISITOR(Declaration);

private:
    node_ptr<spicy::Hook> _hook;
};
}
}

#endif
