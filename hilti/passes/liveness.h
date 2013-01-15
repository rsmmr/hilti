
#ifndef HILTI_PASSES_LIVENESS_H
#define HILTI_PASSES_LIVENESS_H

#include "../pass.h"

namespace hilti {
namespace passes {

class CFG;

/// Computes liveness information for local variables.
class Liveness : public Pass<>
{
public:
    /// Constructor.
    Liveness(CompilerContext* context, shared_ptr<CFG> cfg);
    virtual ~Liveness();

    /// Calculates the liveness information for the module.
    ///
    /// Returns: True if no error occured.
    bool run(shared_ptr<Node> module) override;

    typedef Statement::variable_set variable_set;
    typedef std::pair<shared_ptr<variable_set>, shared_ptr<variable_set>> liveness_sets;

    /// Returns liveness sets for a statement. The first element is all
    /// variables live right before the statement, and the second all live
    /// right after it.
    ///
    /// Must only be called after run() has executed.
    liveness_sets liveness(shared_ptr<Statement> stmt) const;

protected:
    void setLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out);
    void addLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out);

    /// Computes a hash over the current liveness sets suitable for detecting
    /// changes.
    size_t hashLiveness();

    void visit(statement::Block* s);
    void visit(declaration::Function* s);
    void visit(Statement* s) override;

private:
    CompilerContext* _context;
    shared_ptr<CFG> _cfg;
    bool _run = false;

    std::map<shared_ptr<Statement>, liveness_sets> _livenesses;
};

}

}

#endif
