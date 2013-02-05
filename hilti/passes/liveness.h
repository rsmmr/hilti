
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

    struct LivenessSets  {
        shared_ptr<variable_set> in;       // All variables live right before the statement.
        shared_ptr<variable_set> out;      // All variables live right after the statement.
        shared_ptr<variable_set> dead;     // All variables live before this instructio, but dead afterwards.
    };

    typedef std::unordered_map<shared_ptr<Statement>, LivenessSets> liveness_map;

    /// Returns liveness sets for a statement. The first element is all
    /// variables live right before the statement, and the second all live
    /// right after it.
    ///
    /// Must only be called after run() has executed.
    LivenessSets liveness(shared_ptr<Statement> stmt) const;

    /// XXX
    bool have(shared_ptr<Statement> stmt) const;

    /// Returns true if a given parameter is live before executing a statement.
    bool liveIn(shared_ptr<Statement> stmt, shared_ptr<hilti::type::function::Parameter> p) const;

    /// Returns true if a given local is live before executing a statement.
    bool liveIn(shared_ptr<Statement> stmt, shared_ptr<variable::Local> l) const;

    /// Returns true if a given expression is in the "dead" after executing a
    /// statement.
    bool deadOut(shared_ptr<Statement> stmt, shared_ptr<Expression> e) const;

    /// XXX
    bool liveOut(shared_ptr<Statement> stmt, shared_ptr<Expression> e) const;

    /// XXX
    bool liveIn(shared_ptr<Statement> stmt, shared_ptr<Expression> e) const;

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
