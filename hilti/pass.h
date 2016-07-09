
#ifndef HILTI_PASS_H
#define HILTI_PASS_H

#include "common.h"
#include "visitor.h"

namespace hilti {


/// Base class for passes iterating over an AST.
template <typename Result = int, typename Arg1 = int, typename Arg2 = int>
class Pass : public Visitor<Result, Arg1, Arg2> {
protected:
    /// Constructor.
    ///
    /// name: A name for the pass, which will be used for identification in
    /// output messages.
    Pass(const char* name, bool modifier = false)
    {
        this->setLoggerName(string("pass::") + name);
        this->setModifier(modifier);
    }

    virtual ~Pass()
    {
    }

    /// Runs the pass on a given AST.
    ///
    /// ast: The base node of the AST to run on.
    virtual bool run(shared_ptr<Node> ast) = 0;
};
}

#endif
