
#ifndef HILTI_ID_RESOLVER_H
#define HILTI_ID_RESOLVER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Resolves ID references. It replaces nodes of type ID with the node that
/// it's referencing.
class IdResolver : public Pass<> {
public:
    IdResolver() : Pass<>("hilti::IdResolver", true)
    {
    }
    virtual ~IdResolver();

    /// Resolves ID references.
    ///
    /// module: The AST to resolve.
    ///
    /// report_unresolved: If true, IDs that we can't resolve are reported as
    /// errorr; if false, they are simply left untouched.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<Node> module, bool report_unresolved);

protected:
    void visit(expression::ID* i) override;
    void visit(Declaration* d) override;
    void visit(Function* f) override;
    void visit(type::Unknown* t) override;
    void visit(declaration::Variable* d) override;
    void visit(statement::ForEach* s) override;

private:
    bool run(shared_ptr<Node> ast) override
    {
        return false;
    };

    std::set<string> _locals;
    bool _report_unresolved = true;
};
}
}

#endif
