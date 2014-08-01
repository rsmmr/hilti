
#ifndef BINPAC_CODEGEN_SYNCHRONIZER_H
#define BINPAC_CODEGEN_SYNCHRONIZER_H

#include <ast/visitor.h>

#include "../common.h"
#include "cg-visitor.h"

namespace binpac {

namespace codegen {

enum SynchronizerType {
    SynchronizeAt, // Do not consume the mached synchronization object.
    SynchronizeAfter, // Consume the mached synchronization object.
    SynchronizeUnspecified // Placeholder when not yet set.
};

class SynchronizerState {
public:
    SynchronizerType type;              // The type of synchronization.
    shared_ptr<hilti::Expression> data; // The current input bytes object.
    shared_ptr<hilti::Expression> cur;  // The current input position in \a data.
};

/// Generates code to (re-)synchronize parsing based on an upcoming field.
class Synchronizer : public CGVisitor<>
{
public:
    Synchronizer(CodeGen* cg);
    virtual ~Synchronizer();

    /// Generates the code to synchronized streaming at a production. The
    /// production must indicate support for synchronization via
    /// canSynchronize(). The code throws a XXX exception when
    /// synchronization failed.
    ///
    /// data: The current input bytes object.
    ///
    /// cur: The current input position in \a data
    ///
    /// Returns: The new input position.
    shared_ptr<hilti::Expression> hiltiSynchronize(shared_ptr<Production> p, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> cur);

protected:
    SynchronizerState* state();

    void visit(ctor::Bytes* b) override;
    void visit(ctor::RegExp* b) override;

    void visit(expression::Ctor* b) override;

    void visit(production::Literal* l) override;
    void visit(production::Sequence* l) override;
    void visit(production::ChildGrammar* l) override;
    void visit(production::LookAhead* l) override;

private:
    // Wrapper around processOne().
    void _hiltiSynchronizeOne(shared_ptr<Node> node);

    // Synchronizes by finding a byte sequence.
    void _hiltiSynchronizeOnBytes(const string& data);

    // Synchronizes by finding a regular expression.
    void _hiltiSynchronizeOnRegexp(const hilti::builder::regexp::re_pattern_list& patterns);

    // Raises an exception signaling that synchronization has failed.
    void _hiltiSynchronizeError(const string& msg);

    // Gnerates the HILTI code to handle an out-of-input situation by
    // retrying next time if possible (i.e., more input may come).
    void _hiltiNotYetFound(shared_ptr<hilti::builder::BlockBuilder> cont);

    // Prints the given message to the binpac debug stream.
    void _hiltiDebug(const string& msg);

    // Prints the given message to the binpac verbose debug stream.
    void _hiltiDebugVerbose(const string& msg);

    SynchronizerState _state;

};

}
}

#endif
