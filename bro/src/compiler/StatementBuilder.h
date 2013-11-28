///
/// A StatementBuilder compiles a single Bro statement into HILTI code.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_STATEMENT_BUILDER_H
#define BRO_PLUGIN_HILTI_COMPILER_STATEMENT_BUILDER_H

#include "BuilderBase.h"

class AddStmt;
class BreakStmt;
class DelStmt;
class EventStmt;
class ExprStmt;
class FallthroughStmt;
class ForStmt;
class IfStmt;
class InitStmt;
class NextStmt;
class NullStmt;
class PrintStmt;
class ReturnStmt;
class StmtList;
class SwitchStmt;
class When;

namespace bro {
namespace hilti {
namespace compiler {

class ModuleBuilder;

class StatementBuilder : public BuilderBase {
public:
	/**
	 * Constructor.
	 *
	 * mbuilder: The module builder to use.
	 */
	StatementBuilder(class ModuleBuilder* mbuilder);

	/**
	 * Compiles a Bro statement into HILTI code, inserting the code at
	 * the associated module builder's current location.
	 */
	void Compile(const Stmt* stmt);

protected:
	enum FlowState { FLOW_STATE_BREAK, FLOW_STATE_NEXT, FLOW_STATE_FALLTHROUGH };

	typedef std::list<std::pair<FlowState, shared_ptr<::hilti::Expression> > > flow_state_list;

	/**
	 * Record a block to branch to for an upcoming flow statement. This
	 * pushes the given information onto an internal stack; when a flow
	 * statement is encountered, the most recently pushed matching state
	 * entry will be used.
	 *
	 * @param fstate The flow statement upon which to branch to *dst*.
	 *
	 * @param dst The block to branch to.
	 */
	void PushFlowState(FlowState fstate, shared_ptr<::hilti::Expression> dst);

	/**
	 * Removes the most recently pushed flow state from the internal
	 * stack.
	 */
	void PopFlowState();

	/**
	 * Returns the target block associated with most recently pushed flow
	 * state of the given type. It's an internal error if there is none.
	 *
	 * @param The flow statement to search.
	 *
	 * @return The corresponding block to branch to.
	 */
	shared_ptr<::hilti::Expression> CurrentFlowState(FlowState fstate);

	void NotSupported(const ::BroType* type, const char* where);
	void NotSupported(const ::Expr* expr, const char* where);
	void NotSupported(const ::Stmt* stmt);

	void Compile(const ::AddStmt* stmt);
	void Compile(const ::BreakStmt* stmt);
	void Compile(const ::DelStmt* stmt);
	void Compile(const ::EventStmt* stmt);
	void Compile(const ::ExprStmt* stmt);
	void Compile(const ::FallthroughStmt* stmt);
	void Compile(const ::ForStmt* stmt);
	void Compile(const ::IfStmt* stmt);
	void Compile(const ::InitStmt* stmt);
	void Compile(const ::NextStmt* stmt);
	void Compile(const ::NullStmt* stmt);
	void Compile(const ::PrintStmt* stmt);
	void Compile(const ::ReturnStmt* stmt);
	void Compile(const ::StmtList* stmt);
	void Compile(const ::SwitchStmt* stmt);
	void Compile(const ::WhenStmt* stmt);

private:
	flow_state_list flow_stack;
};

}
}
}

#endif
