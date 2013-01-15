
#ifndef HILTI_PASSES_CFG_H
#define HILTI_PASSES_CFG_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Builds a control flow graph.
///
/// TODO: This is quite simple yet and a number of things are left to do. We
/// only link instructions not block, because that a bit tricky with our
/// nested structure. We don't mark entry and exit point particularly so this
/// is good only for intra-prodecural analysis. We don't know what exceptions
/// instructions can throw and hence need to do lots of worst case edges
/// inside try blocks.
class CFG : public Pass<>
{
public:
    /// Constructor.
    CFG(CompilerContext* context);
    virtual ~CFG();

    /// Calculates the control flow graph for module.
    ///
    /// Returns: True if no error occured.
    bool run(shared_ptr<Node> module) override;

    /// Returns all predecessors of a statement.  Note that this operates
    /// differently for blocks and non-block statements: the predecessors of
    /// a block will be other blocks, while that of a statement inside a
    /// block will be the previous statement (or null for the first).
    /// 
    /// Must only be called after run().
    ///
    /// TODO: The block chaining does't work right yet and disabled. Indeed
    /// it's not clear if it can with the current structure.
    shared_ptr<std::set<shared_ptr<Statement>>> predecessors(shared_ptr<Statement> stmt);

    /// Returns all successors of a statement.  Note that this operates
    /// differently for blocks and non-block statements: the successors of a
    /// block will be other blocks, while that of a statement inside a block
    /// will be the next statement (or null for the last).
    ///
    /// Must only be called after run().
    ///
    /// TODO: The block chaining does't work right yet and disabled. Indeed
    /// it's not clear if it can with the current structure.
    shared_ptr<std::set<shared_ptr<Statement>>> successors(shared_ptr<Statement> stmt);

protected:
    void addSuccessor(Statement* stmt, shared_ptr<Statement> succ);
    void addSuccessor(shared_ptr<Statement> stmt, shared_ptr<Statement> succ);
    void setPredecessors(shared_ptr<Statement> stmt, std::set<shared_ptr<Statement>> preds);
    void setSuccessors(shared_ptr<Statement> stmt, std::set<shared_ptr<Statement>> succs);

    void visit(declaration::Function* f) override;
    void visit(statement::Block* s) override;
    void visit(statement::Try* s) override;
    void visit(statement::instruction::Resolved* s) override;
    void visit(statement::instruction::Unresolved* s) override;
    void visit(statement::try_::Catch* s) override;
    void visit(statement::ForEach* s) override;

private:
    CompilerContext* _context;
    bool _run = false;
    bool _changed = false;
    int _pass = 0;

    std::list<shared_ptr<Statement>> _siblings;

    typedef std::map<shared_ptr<Statement>, shared_ptr<std::set<shared_ptr<Statement>>>> stmt_map;

    stmt_map _predecessors;
    stmt_map _successors;
};

}

}

#endif
