///
/// A Spicy compile-time unit.
///

#ifndef SPICY_MODULE_H
#define SPICY_MODULE_H

#include <ast/module.h>
#include <ast/visitor.h>

#include "common.h"

namespace spicy {

class CompilerContext;

/// AST node for a top-level module.
class Module : public ast::Module<AstInfo> {
    AST_RTTI
public:
    /// Constructor.
    ///
    /// ctx: The context this module is part of.
    ///
    /// id: A non-scoped ID with the module's name.
    ///
    /// path: A file system path associated with the module.
    ///
    /// l: Associated location.
    Module(CompilerContext* ctx, shared_ptr<ID> id, const string& path = "-",
           const Location& l = Location::None);

    /// Adds a property to the module.
    ///
    /// prop: The property to add.
    void addProperty(shared_ptr<Attribute> prop);

    /// Returns a list of all properties.
    std::list<shared_ptr<Attribute>> properties() const;

    /// Returns the property of a given name, or null if no such. If there
    /// are more than one of that name, returns the last.
    shared_ptr<Attribute> property(const string& prop) const;

    CompilerContext* context() const;

    ACCEPT_VISITOR_ROOT();

private:
    CompilerContext* _context;

    std::list<node_ptr<Attribute>> _properties;
};
}

#endif
