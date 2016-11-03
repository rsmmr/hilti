
#ifndef SPICY_FUNCTION_H
#define SPICY_FUNCTION_H

#include <ast/function.h>
#include <ast/visitor.h>

#include "ast-info.h"
#include "common.h"

namespace spicy {

/// AST node for a function.
class Function : public ast::Function<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// id: A non-scoped ID with the function's name.
    ///
    /// module: The module the function is part of. Note that it will not
    /// automatically be added to that module.
    ///
    /// body: A statement with the function's body. Typically, the statement type will be that of a
    /// block of statements.
    ///
    /// l: Location associated with the node.
    Function(shared_ptr<ID> id, shared_ptr<spicy::type::Function> ftype, shared_ptr<Module> module,
             shared_ptr<spicy::Statement> body = nullptr, const Location& l = Location::None);

    /// Associates a HILTI-level name with this function. That name will be
    /// used instead of the default one during code generation.
    void setHiltiFunctionID(shared_ptr<ID> id);

    /// Returns the HILTI-level name associated with this function, if any.
    shared_ptr<ID> hiltiFunctionID() const;

    ACCEPT_VISITOR_ROOT();

private:
    shared_ptr<ID> _hilti_id = nullptr;
};

/// AST node for a hook. Note that we don't derive this from a function as in
/// Spicy hooks are actually quite differnt.
class Hook : public Node {
    AST_RTTI
public:
    /// The kind of a hook defines whether it's to trigger during parsing or
    /// composing, or both.
    enum Kind {
        PARSE,         ///< A parse hook.
        COMPOSE,       ///< A compose hook.
        PARSE_COMPOSE, ///< A hook trigering during both parsing and composing.
    };

    /// body: A statement with the hook's body. Typically, the statement type
    /// will be that of a block of statements. body can be null if this is
    /// just a hook prototype.
    ///
    /// kind: The kind of the hook, i.e., whether it's to trigger during
    /// parsing or composing.
    ///
    /// prio: The priority of the hook. If multiple hook are defined for the
    /// same field, they are executed in order of decreasing priority.
    ///
    /// debug: If True, this hook will be only compiled in if the code
    /// generator is including debug code, and it will only be executed if at
    /// run-time, debug mode is enabled (via the C function
    /// ~~spicy_enable_debug).
    ///
    /// args: Arguments, if the hook takes any, or an empty list otherwise.
    ///
    /// l: Location associated with the node.
    Hook(shared_ptr<spicy::Statement> body, Kind kind, int prio = 0, bool debug = false,
         bool foreach = false, parameter_list args = parameter_list(),
         const Location& l = Location::None);

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
    bool foreach () const;

    /// Returns true if this is a hook to run during parsing (as opposed to
    /// composing, but note that a hook can be both).
    bool parseHook() const;

    /// Returns true if this is a hook to run during composing (as opposed to
    /// parsing, but note that a hook can be both).
    bool composeHook() const;

    /// Returns the hooks paramaters. The list will be empty if it doesn't
    /// take any.
    parameter_list parameters() const;

    /// Sets the unit type that this hook is part of. Should be called only
    /// form the resolver passes.
    void setUnit(shared_ptr<type::Unit> unit);

    /// Turns this hook into a debug hook that executes only when compiled in
    /// debugging mode.
    void setDebug();

    ACCEPT_VISITOR_ROOT()

private:
    node_ptr<spicy::Statement> _body;
    node_ptr<type::Unit> _unit = nullptr;
    Kind _kind;
    int _prio;
    bool _debug;
    bool _foreach;
    std::list<node_ptr<type::function::Parameter>> _args;
};
}

#endif
