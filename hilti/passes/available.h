
#ifndef HILTI_PASSES_AVAILABLE_H
#define HILTI_PASSES_AVAILABLE_H

#include <unordered_map>

#include "../pass.h"

namespace hilti {
namespace passes {

class CFG;

/// Computes available expression (instruction) information for functions.
class Available : public Pass<> {
public:
    // Representation of one expression that might be available.
    struct AExpression {
        AExpression(shared_ptr<Instruction> instr);
        AExpression& operator=(const AExpression& other);
        bool operator==(const AExpression& other) const; // Compares RHS of instructions only.
        bool operator<(const AExpression& other) const;

        shared_ptr<statement::Instruction> instr;

        FlowVariable where;        // Variable where the value of the expression is available.
        FlowVariable dependencies; // All variables that this instruction depends on.
    };

    typedef std::vector<AExpression> available_set;
    typedef std::unordered_map<shared_ptr<Statement>, AvailableSets> available_map;

    struct AExpressionSets {
        shared_ptr<variable_set> in;  // All variables live right before the statement.
        shared_ptr<variable_set> out; // All variables live right after the statement.
    };

    /// Constructor.
    Available(CompilerContext* context, shared_ptr<CFG> cfg);
    virtual ~Available();

    /// Calculates the Available information for the module.
    ///
    /// Returns: True if no error occured.
    bool run(shared_ptr<Node> module) override;

    /// Returns Available sets for a statement. The first element is all
    /// variables live right before the statement, and the second all live
    /// right after it.
    ///
    /// Must only be called after run() has executed.
    AvailableSets Available(shared_ptr<Statement> stmt) const;

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
    void setAvailable(shared_ptr<Statement> stmt, variable_set in, variable_set out);
    void addAvailable(shared_ptr<Statement> stmt, variable_set in, variable_set out);
    size_t hashAvailable();
    void dumpDebug(int round);

private:
    CompilerContext* _context;
    shared_ptr<Module> _module = nullptr;
    shared_ptr<CFG> _cfg;
    Available_map _Availablees;
};
}
}

#endif
