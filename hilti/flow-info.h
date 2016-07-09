
#ifndef HILTI_FLOW_INFO_H
#define HILTI_FLOW_INFO_H

#include <memory>
#include <set>
#include <string>

using std::shared_ptr;
using std::string;

namespace hilti {

class Statement;
class Expression;
class ID;

namespace statement {
class Block;
}
namespace expression {
class Variable;
class Parameter;
}

/// Structure to describe a variable for data flow analysis.
///
/// \todo: Right now we model only local variables and parameters and
/// hence the names need to be unique only within functions. If we add
/// globals, we need to change that.
struct FlowVariable {
    FlowVariable(shared_ptr<expression::Variable> variable);
    FlowVariable(shared_ptr<expression::Parameter> param);

    FlowVariable& operator=(const FlowVariable& other);
    bool operator==(const FlowVariable& other) const;
    bool operator<(const FlowVariable& other) const;

    string name;       /// Name for this variable unique within its function. Used for comparision.
    shared_ptr<ID> id; /// ID referencing this variable.
    shared_ptr<Expression> expression; /// Expression to reference the variable.
};

struct FlowVariablePtrCmp {
    bool operator()(const shared_ptr<FlowVariable>& f1, const shared_ptr<FlowVariable>& f2);
};

extern bool operator==(const shared_ptr<FlowVariable>& v1, const shared_ptr<FlowVariable>& v2);

/// Provides data and control flow information for the statement.
struct FlowInfo {
    typedef std::set<shared_ptr<FlowVariable>, FlowVariablePtrCmp> variable_set;

    /// Variables defined by this instruction, i.e., overwritten with a
    /// new valid value.
    variable_set defined;

    /// Variables cleared by this instruction, i.e., redefined to no
    /// longer have a valid value.
    variable_set cleared;

    /// Variables that have their current value potentially modified by
    /// this instruction, but not reset/cleared to a new independent value/
    variable_set modified;

    /// Variables potentially read by this instructionm but not modified.
    /// This is a superset of \a defined and \a cleared.
    variable_set read;

    // All potential successor blocks as determined by any of the
    // statements operands.
    std::set<shared_ptr<statement::Block>> successors;
};
}

#endif
