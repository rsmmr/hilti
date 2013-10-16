///
/// A ExpressionBuilder compiles a single Bro expression into a HILTI expression.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_EXPRESSION_BUILDER_H
#define BRO_PLUGIN_HILTI_COMPILER_EXPRESSION_BUILDER_H

#include "BuilderBase.h"

class AddExpr;
class AddToExpr;
class ArithCoerceExpr;
class AssignExpr;
class CallExpr;
class CloneExpr;
class CondExpr;
class ConstExpr;
class DivideExpr;
class EventExpr;
class FieldExpr;
class FieldAssignExpr;
class FlattenExpr;
class HasFieldExpr;
class InExpr;
class IndexExpr;
class ListExpr;
class ModExpr;
class NameExpr;
class NegExpr;
class NotExpr;
class PosExpr;
class RecordCoerceExpr;
class RecordConstructorExpr;
class RefExpr;
class RemoveFromExpr;
class ScheduleExpr;
class SetConstructorExpr;
class SizeExpr;
class SubExpr;
class TableCoerceExpr;
class TableConstructorExpr;
class TimesExpr;
class VectorCoerceExpr;
class VectorConstructorExpr;
class IncrExpr;
class BoolExpr;
class EqExpr;
class RelExpr;
class InExpr;

namespace hilti { class Expression; }

namespace bro {
namespace hilti {
namespace compiler {

class ExpressionBuilder : public BuilderBase {
public:
	/**
	 * Constructor.
	 *
	 * mbuilder: The module builder to use.
	 */
	ExpressionBuilder(class ModuleBuilder* mbuilder);

	/**
	 * Compiles a Bro expression into a HILTI expression, inserting the
	 * code at the associated module builder's current location.
	 *
	 * @return The compiled expression.
	 */
	std::shared_ptr<::hilti::Expression> Compile(const ::Expr* expr);

protected:
	std::shared_ptr<::hilti::Expression> Compile(const ::AddExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::AddToExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::ArithCoerceExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::AssignExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::BoolExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::CallExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::CloneExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::CondExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::ConstExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::DivideExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::EqExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::EventExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::FieldAssignExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::FieldExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::FlattenExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::HasFieldExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::InExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::IncrExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::IndexExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::ListExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::ModExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::NameExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::NegExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::NotExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::PosExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::RecordCoerceExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::RecordConstructorExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::RefExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::RelExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::RemoveFromExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::ScheduleExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::SetConstructorExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::SizeExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::SubExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::TableCoerceExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::TableConstructorExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::TimesExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::VectorCoerceExpr* expr);
	std::shared_ptr<::hilti::Expression> Compile(const ::VectorConstructorExpr* expr);
};

}
}
}

#endif
