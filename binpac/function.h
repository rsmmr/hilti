
#ifndef BINPAC_FUNCTION_H
#define BINPAC_FUNCTION_H

#include <ast/function.h>
#include <ast/visitor.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {

/// AST node for a function.
class Function : public ast::Function<AstInfo>
{
public:
    /// Constructor.
    ///
    /// id: A non-scoped ID with the function's name.
    ///
    /// module: The module the function is part of. Note that it will not
    /// automatically be added to that module.
    ///
    /// body: A statement with the function's body. Typically, the statement type will be that of a block of statements.
    ///
    /// l: Location associated with the node.
    Function(shared_ptr<ID> id, shared_ptr<binpac::type::Function> ftype, shared_ptr<Module> module, shared_ptr<binpac::Statement> body = nullptr, const Location& l=Location::None);

    ACCEPT_VISITOR_ROOT();
};

/// AST node for a hook. Note that we don't derive this from a function as in
/// BinPAC++ hooks are actually quite differnt.
class Hook : public Node
{
public:
    /// body: A statement with the hook's body. Typically, the statement type
    /// will be that of a block of statements. body can be null if this is
    /// just a hook prototype.
    ///
    /// prio: The priority of the hook. If multiple hook are defined for the
    /// same field, they are executed in order of decreasing priority.
    /// 
    /// debug: If True, this hook will be only compiled in if the code
    /// generator is including debug code, and it will only be executed if at
    /// run-time, debug mode is enabled (via the C function
    /// ~~binpac_enable_debug).
    ///
    /// l: Location associated with the node.
    Hook(shared_ptr<binpac::Statement> body = nullptr,
         int prio = 0, bool debug = false,
         bool foreach = false, const Location& l=Location::None);

    /// Returns the hook's body.
    shared_ptr<statement::Block> body() const;

    /// Returns the unit type the hook is part of. Note that this is
    /// initially unset until the resolver passes have run.
    shared_ptr<type::Unit> unit() const;

    /// Returns the hook's priority.
    int priority() const;

    /// Returns true if this is a hook to execute only in debugging mode.
    bool debug() const;

    /// Returns true if this hooks is marked with \c foreach.
    bool foreach() const;

    /// Sets the unit type that this hook is part of. Should be called only
    /// form the resolver passes.
    void setUnit(shared_ptr<type::Unit> unit);

    /// Turns this hook into a debug hook that executes only when compiled in
    /// debugging mode.
    void setDebug();

    ACCEPT_VISITOR_ROOT()

private:
    node_ptr<binpac::Statement> _body;
    node_ptr<type::Unit> _unit = nullptr;
    int _prio;
    bool _debug;
    bool _foreach;
};

}

#endif
