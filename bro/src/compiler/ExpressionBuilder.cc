
#include <Expr.h>
#undef List

#include <hilti/hilti.h>
#include <util/util.h>

#include "ExpressionBuilder.h"

using namespace bro::hilti::compiler;

ExpressionBuilder::ExpressionBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

shared_ptr<hilti::Expression> ExpressionBuilder::Compile(const ::Expr* expr)
	{
	switch ( expr->Tag() ) {
	case EXPR_ADD:
		return Compile(static_cast<const ::AddExpr*>(expr));

	case EXPR_ADD_TO:
		return Compile(static_cast<const ::AddToExpr*>(expr));

	case EXPR_ARITH_COERCE:
		return Compile(static_cast<const ::ArithCoerceExpr*>(expr));

	case EXPR_ASSIGN:
		return Compile(static_cast<const ::AssignExpr*>(expr));

	case EXPR_CALL:
		return Compile(static_cast<const ::CallExpr*>(expr));

	case EXPR_CLONE:
		return Compile(static_cast<const ::CloneExpr*>(expr));

	case EXPR_COND:
		return Compile(static_cast<const ::CondExpr*>(expr));

	case EXPR_CONST:
		return Compile(static_cast<const ::ConstExpr*>(expr));

	case EXPR_DIVIDE:
		return Compile(static_cast<const ::DivideExpr*>(expr));

	case EXPR_EVENT:
		return Compile(static_cast<const ::EventExpr*>(expr));

	case EXPR_FIELD:
		return Compile(static_cast<const ::FieldExpr*>(expr));

	case EXPR_FIELD_ASSIGN:
		return Compile(static_cast<const ::FieldAssignExpr*>(expr));

	case EXPR_FLATTEN:
		return Compile(static_cast<const ::FlattenExpr*>(expr));

	case EXPR_HAS_FIELD:
		return Compile(static_cast<const ::HasFieldExpr*>(expr));

	case EXPR_IN:
		return Compile(static_cast<const ::InExpr*>(expr));

	case EXPR_INDEX:
		return Compile(static_cast<const ::IndexExpr*>(expr));

	case EXPR_LIST:
		return Compile(static_cast<const ::ListExpr*>(expr));

	case EXPR_MOD:
		return Compile(static_cast<const ::ModExpr*>(expr));

	case EXPR_NAME:
		return Compile(static_cast<const ::NameExpr*>(expr));

	case EXPR_NEGATE:
		return Compile(static_cast<const ::NegExpr*>(expr));

	case EXPR_NOT:
		return Compile(static_cast<const ::NotExpr*>(expr));

	case EXPR_POSITIVE:
		return Compile(static_cast<const ::PosExpr*>(expr));

	case EXPR_RECORD_COERCE:
		return Compile(static_cast<const ::RecordCoerceExpr*>(expr));

	case EXPR_RECORD_CONSTRUCTOR:
		return Compile(static_cast<const ::RecordConstructorExpr*>(expr));

	case EXPR_REF:
		return Compile(static_cast<const ::RefExpr*>(expr));

	case EXPR_REMOVE_FROM:
		return Compile(static_cast<const ::RemoveFromExpr*>(expr));

	case EXPR_SCHEDULE:
		return Compile(static_cast<const ::ScheduleExpr*>(expr));

	case EXPR_SET_CONSTRUCTOR:
		return Compile(static_cast<const ::SetConstructorExpr*>(expr));

	case EXPR_SIZE:
		return Compile(static_cast<const ::SizeExpr*>(expr));

	case EXPR_SUB:
		return Compile(static_cast<const ::SubExpr*>(expr));

	case EXPR_TABLE_COERCE:
		return Compile(static_cast<const ::TableCoerceExpr*>(expr));

	case EXPR_TABLE_CONSTRUCTOR:
		return Compile(static_cast<const ::TableConstructorExpr*>(expr));

	case EXPR_TIMES:
		return Compile(static_cast<const ::TimesExpr*>(expr));

	case EXPR_VECTOR_COERCE:
		return Compile(static_cast<const ::VectorCoerceExpr*>(expr));

	case EXPR_VECTOR_CONSTRUCTOR:
		return Compile(static_cast<const ::VectorConstructorExpr*>(expr));

	case EXPR_DECR:
	case EXPR_INCR:
		return Compile(static_cast<const ::IncrExpr*>(expr));

	case EXPR_AND:
	case EXPR_OR:
		return Compile(static_cast<const ::BoolExpr*>(expr));

	case EXPR_EQ:
	case EXPR_NE:
		return Compile(static_cast<const ::EqExpr*>(expr));

	case EXPR_GE:
	case EXPR_GT:
	case EXPR_LE:
	case EXPR_LT:
		return Compile(static_cast<const ::RelExpr*>(expr));

	default:
		Error(::util::fmt("unsupported expression %s", ::expr_name(expr->Tag())));
	}

	// Cannot be reached.
	return nullptr;
	}


shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AddExpr* expr)
	{
	Error("no support yet for compiling AddExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AddToExpr* expr)
	{
	Error("no support yet for compiling AddToExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ArithCoerceExpr* expr)
	{
	Error("no support yet for compiling ArithCoerceExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AssignExpr* expr)
	{
	Error("no support yet for compiling AssignExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CallExpr* expr)
	{
	auto ftype = expr->Func()->Type()->AsFuncType();
	auto func = HiltiExpression(expr->Func());
	auto args = HiltiExpression(expr->Args());

	auto ytype = ftype->YieldType();

	if ( ytype && ytype->Tag() != TYPE_VOID && ytype->Tag() != TYPE_ANY )
		{
		auto result = Builder()->addTmp("result", HiltiType(ftype->YieldType()));
		Builder()->addInstruction(result, ::hilti::instruction::flow::CallResult, func, args);
		return result;
		}

	else
		{
		Builder()->addInstruction(::hilti::instruction::flow::CallVoid, func, args);
		return nullptr;
		}
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CloneExpr* expr)
	{
	Error("no support yet for compiling CloneExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CondExpr* expr)
	{
	Error("no support yet for compiling CondExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ConstExpr* expr)
	{
	return HiltiValue(expr->Value());
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::DivideExpr* expr)
	{
	Error("no support yet for compiling DivideExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::EventExpr* expr)
	{
	Error("no support yet for compiling EventExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::FieldExpr* expr)
	{
	Error("no support yet for compiling FieldExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::FieldAssignExpr* expr)
	{
	Error("no support yet for compiling FieldAssignExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::FlattenExpr* expr)
	{
	Error("no support yet for compiling FlattenExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::HasFieldExpr* expr)
	{
	Error("no support yet for compiling HasFieldExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::InExpr* expr)
	{
	Error("no support yet for compiling InExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::IndexExpr* expr)
	{
	Error("no support yet for compiling IndexExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ListExpr* expr)
	{
	::hilti::builder::tuple::element_list elems;

	const ::expr_list& exprs = expr->Exprs();

	loop_over_list(exprs, i)
		{
		auto hexpr = HiltiExpression(exprs[i]);
		elems.push_back(hexpr);
		}

	return ::hilti::builder::tuple::create(elems);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ModExpr* expr)
	{
	Error("no support yet for compiling ModExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::NameExpr* expr)
	{
	auto id = expr->Id();

	if ( id->IsGlobal() )
		{
                auto val = id->ID_Val();
		assert(val);

		if ( id->IsConst() )
			return HiltiValue(val);

		// TODO: This is technically not correct as global functions
		// aren't constants. If somebody reassigs a global function
		// this will not account for that.
		if ( val->Type()->Tag() == TYPE_FUNC )
			return HiltiValue(val);
		}

	return ::hilti::builder::id::create(HiltiSymbol(id));
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::NegExpr* expr)
	{
	Error("no support yet for compiling NegExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::NotExpr* expr)
	{
	Error("no support yet for compiling NotExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::PosExpr* expr)
	{
	Error("no support yet for compiling PosExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RecordCoerceExpr* expr)
	{
	Error("no support yet for compiling RecordCoerceExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RecordConstructorExpr* expr)
	{
	Error("no support yet for compiling RecordConstructorExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RefExpr* expr)
	{
	Error("no support yet for compiling RefExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RemoveFromExpr* expr)
	{
	Error("no support yet for compiling RemoveFromExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ScheduleExpr* expr)
	{
	Error("no support yet for compiling ScheduleExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::SetConstructorExpr* expr)
	{
	Error("no support yet for compiling SetConstructorExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::SizeExpr* expr)
	{
	Error("no support yet for compiling SizeExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::SubExpr* expr)
	{
	Error("no support yet for compiling SubExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TableCoerceExpr* expr)
	{
	Error("no support yet for compiling TableCoerceExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TableConstructorExpr* expr)
	{
	Error("no support yet for compiling TableConstructorExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TimesExpr* expr)
	{
	Error("no support yet for compiling TimesExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::VectorCoerceExpr* expr)
	{
	Error("no support yet for compiling VectorCoerceExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::VectorConstructorExpr* expr)
	{
	Error("no support yet for compiling VectorConstructorExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::IncrExpr* expr)
	{
	Error("no support yet for compiling IncrExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::BoolExpr* expr)
	{
	Error("no support yet for compiling BoolExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::EqExpr* expr)
	{
	Error("no support yet for compiling EqExpr", expr);
	return 0;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RelExpr* expr)
	{
	Error("no support yet for compiling RelExpr", expr);
	return 0;
	}
