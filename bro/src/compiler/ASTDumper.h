///
/// Dumps a Bro AST to stderr for debugging.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_BRO_AST_DUMPER_H
#define BRO_PLUGIN_HILTI_COMPILER_BRO_AST_DUMPER_H

#include "Traverse.h"

class Stmt;
class Expr;
class ID;
class BroObj;

namespace bro {
namespace hilti {
namespace compiler {

/// Dumps a Bro (sub-)AST to standard error.
class ASTDumper : public TraversalCallback {
public:
	static void Dump(const ::Func *func);
	static void Dump(const ::Stmt *stmt);
	static void Dump(const ::Expr *expr);
	static void Dump(const ::ID *id);

protected:
	TraversalCode PreFunction(const ::Func* fund) override;
	TraversalCode PreStmt(const ::Stmt* stmt) override;
		TraversalCode PreExpr(const ::Expr* expr) override;
	TraversalCode PreID(const ::ID* id) override;
	TraversalCode PreTypedef(const ::ID* id) override;
	TraversalCode PreDecl(const ::ID* id) override;
	TraversalCode PostFunction(const ::Func* fund) override;
	TraversalCode PostStmt(const ::Stmt* stmt) override;
	TraversalCode PostExpr(const ::Expr* expr) override;
	TraversalCode PostID(const ::ID* id) override;
	TraversalCode PostTypedef(const ::ID* id) override;
	TraversalCode PostDecl(const ::ID* id) override;

private:
	TraversalCode DoPre(const char* tag, const ::BroObj* obj);
	TraversalCode DoPost(const char* tag, const ::BroObj* obj);

	int depth = 0;
};

}
}
}

#endif
