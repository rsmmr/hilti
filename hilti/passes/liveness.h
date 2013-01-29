
#ifndef HILTI_PASSES_LIVENESS_H
#define HILTI_PASSES_LIVENESS_H

#include <unordered_map>

#include "../pass.h"

namespace hilti {
namespace passes {

class CFG;
class LocalLiveness;

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
    typedef std::unordered_map<shared_ptr<Statement>, liveness_sets> liveness_map;

    /// Returns liveness sets for a statement. The first element is all
    /// variables live right before the statement, and the second all live
    /// right after it.
    ///
    /// Must only be called after run() has executed.
    liveness_sets liveness(shared_ptr<Statement> stmt) const;

protected:
    void processStatement(shared_ptr<Statement> s);
    void setLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out);
    void addLiveness(shared_ptr<Statement> stmt, variable_set in, variable_set out);
    size_t hashLiveness();

private:
    CompilerContext* _context;
    shared_ptr<CFG> _cfg;
    bool _run = false;

    liveness_map _livenesses;
};

}

}

#endif
