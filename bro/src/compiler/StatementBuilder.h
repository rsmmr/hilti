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
};

}
}
}

#endif
