
#ifndef HILTI_GLOBAL_TYPE_RESOLVER_H
#define HILTI_GLOBAL_TYPE_RESOLVER_H

#include <map>

#include "../pass.h"

namespace hilti {
namespace passes {

/// Resolves global type references as good as it can. It replaces nodes of
/// type ID with the node that it's referencing.
class GlobalTypeResolver : public Pass<> {
public:
    GlobalTypeResolver() : Pass<>("hilti::GlobalTypeResolver", true)
    {
    }
    virtual ~GlobalTypeResolver();

    /// Resolves global type references to the degree possible.
    ///
    /// module: The AST to resolve.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<Node> module) override;

protected:
    void visit(declaration::Type* t) override;
    void visit(type::Unknown* t) override;

private:
    int _pass = 0;
    std::shared_ptr<Module> _module;
    std::map<std::string, std::shared_ptr<Type>> _types;
};
}
}

#endif
