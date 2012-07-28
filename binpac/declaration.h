
#ifndef BINPAC_DECLARATION_H
#define BINPAC_DECLARATION_H

#include <ast/declaration.h>
#include <ast/visitor.h>

#include "common.h"

namespace binpac {

/// Base class for AST declaration nodes.
class Declaration : public ast::Declaration<AstInfo>
{
public:
    /// Constructor.
    ///
    /// id: The name of declared object.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: An associated location.
    Declaration(shared_ptr<binpac::ID> id, Linkage linkage, const Location& l=Location::None);

    ACCEPT_VISITOR_ROOT();
};

namespace declaration {

/// AST node for declaring a variable.
class Variable : public binpac::Declaration, public ast::declaration::mixin::Variable<AstInfo>
{
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
    Variable(shared_ptr<binpac::ID> id, Linkage linkage, shared_ptr<binpac::Variable> var, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Declaration);
};

/// AST node for declaring a constant.
class Constant : public binpac::Declaration, public ast::declaration::mixin::Constant<AstInfo>
{
public:
    /// Constructor.
    ///
    /// id: The name of declared constant.
    /// 
    /// linkage: The declaration's linkage.
    ///
    /// constant: The declared constant.
    ///
    /// l: An associated location.
    Constant(shared_ptr<binpac::ID> id, Linkage linkage, shared_ptr<binpac::Constant> constant, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Declaration);
};

/// AST node for declaring a type.
class Type : public binpac::Declaration, public ast::declaration::mixin::Type<AstInfo>
{
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
    Type(shared_ptr<binpac::ID> id, Linkage linkage, shared_ptr<binpac::Type> type, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Declaration);
};

/// AST node for declaring a function.
class Function : public binpac::Declaration, public ast::declaration::mixin::Function<AstInfo>
{
public:
    /// Constructor.
    ///
    /// func: The declared function.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: An associated location.
    Function(shared_ptr<binpac::Function> func, Linkage linkage, const Location& l=Location::None);

    ACCEPT_VISITOR(binpac::Declaration);
};

/// AST node for declaring a hook.
class Hook : public Function
{
public:
    /// Constructor.
    ///
    /// hook: The declared hook.
    ///
    /// linkage: The declaration's linkage.
    ///
    /// l: An associated location.
    Hook(shared_ptr<binpac::Hook> hook, Linkage linkage, const Location& l=Location::None);

    /// Returns the declared hook.
    shared_ptr<binpac::Hook> hook() const;

    ACCEPT_VISITOR(Function);
};

}

}

#endif
