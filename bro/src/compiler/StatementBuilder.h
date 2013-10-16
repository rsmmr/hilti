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
	void Compile(Stmt* stmt);

protected:
	void Compile(::AddStmt* stmt);
	void Compile(::BreakStmt* stmt);
	void Compile(::DelStmt* stmt);
	void Compile(::EventStmt* stmt);
	void Compile(::ExprStmt* stmt);
	void Compile(::FallthroughStmt* stmt);
	void Compile(::ForStmt* stmt);
	void Compile(::IfStmt* stmt);
	void Compile(::InitStmt* stmt);
	void Compile(::NextStmt* stmt);
	void Compile(::NullStmt* stmt);
	void Compile(::PrintStmt* stmt);
	void Compile(::ReturnStmt* stmt);
	void Compile(::StmtList* stmt);
	void Compile(::SwitchStmt* stmt);
	void Compile(::WhenStmt* stmt);
};

}
}
}

#endif
