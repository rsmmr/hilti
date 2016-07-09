
#ifndef HILTI_PASSES_INSTRUCTION_NORMALIZER_H
#define HILTI_PASSES_INSTRUCTION_NORMALIZER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Normalizes instructions by replacing nested ones with flattened versions.
class InstructionNormalizer : public Pass<> {
public:
    InstructionNormalizer();
    bool run(shared_ptr<hilti::Node> module) override;

protected:
    shared_ptr<expression::Variable> _addLocal(shared_ptr<statement::Block> block,
                                               const string& hint, shared_ptr<Type> type,
                                               bool force_name = false,
                                               const Location& l = Location::None);
    std::pair<shared_ptr<statement::Block>, shared_ptr<Expression>> _addBlock(
        shared_ptr<statement::Block> parent, const string& hint, bool dont_add_yet = false,
        const Location& l = Location::None);
    void _addInstruction(shared_ptr<statement::Block> block, shared_ptr<Expression> target = 0,
                         shared_ptr<Instruction> ins = 0, shared_ptr<Expression> op1 = 0,
                         shared_ptr<Expression> op2 = 0, shared_ptr<Expression> op3 = 0,
                         const Location& l = Location::None);

    virtual void visit(declaration::Function* f) override;
    virtual void visit(Module* f) override;
    virtual void visit(statement::ForEach* s) override;
    virtual void visit(statement::Try* s) override;

private:
    std::set<string> _names;
};
}
}

#endif
