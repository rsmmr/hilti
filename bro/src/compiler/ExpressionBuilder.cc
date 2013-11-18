
#include <Expr.h>
#undef List

#include <hilti/hilti.h>
#include <util/util.h>

#include "ExpressionBuilder.h"

using namespace bro::hilti::compiler;

#define UNARY_OP_FUNC \
	[&] (const ::UnaryExpr* expr, \
	     shared_ptr<::hilti::Expression> dst, \
	     shared_ptr<::hilti::Expression> op) \

#define BINARY_OP_FUNC \
	[&] (const ::BinaryExpr* expr, \
	     shared_ptr<::hilti::Expression> dst, \
	     shared_ptr<::hilti::Expression> op1, \
	     shared_ptr<::hilti::Expression> op2) \

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

void ExpressionBuilder::UnsupportedExpression(const ::UnaryExpr* expr)
	{
	Error(::util::fmt("unary %s not supported for operand of type %s",
			  ::expr_name(expr->Tag()),
			  ::type_name(expr->Op()->Type()->Tag())),
	     expr);
	}

void ExpressionBuilder::UnsupportedExpression(const ::BinaryExpr* expr)
	{
	Error(::util::fmt("binary %s not supported for operands of types %s and %s",
			  ::expr_name(expr->Tag()),
			  ::type_name(expr->Op1()->Type()->Tag()),
			  ::type_name(expr->Op2()->Type()->Tag())),
	     expr);
	}

bool ExpressionBuilder::CompileOperator(const ::UnaryExpr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op, shared_ptr<::hilti::Instruction> ins)
	{
	if ( expr->Op()->Type()->Tag() != op )
		return false;

	auto hop = HiltiExpression(expr->Op());
	Builder()->addInstruction(dst, ins, hop);
	return true;
	}

bool ExpressionBuilder::CompileOperator(const ::UnaryExpr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op, unary_op_func func)
	{
	if ( expr->Op()->Type()->Tag() != op )
		return false;

	auto hop = HiltiExpression(expr->Op());
	func(expr, dst, hop);
	return true;
	}

bool ExpressionBuilder::CompileOperator(const ::BinaryExpr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op1, ::TypeTag op2, shared_ptr<::hilti::Instruction> ins)
	{
	if ( expr->Op1()->Type()->Tag() != op1 || expr->Op2()->Type()->Tag() != op2 )
		return false;

	auto hop1 = HiltiExpression(expr->Op1());
	auto hop2 = HiltiExpression(expr->Op2());

	Builder()->addInstruction(dst, ins, hop1, hop2);
	return true;
	}

bool ExpressionBuilder::CompileOperator(const ::BinaryExpr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op1, ::TypeTag op2, binary_op_func func)
	{
	if ( expr->Op1()->Type()->Tag() != op1 || expr->Op2()->Type()->Tag() != op2 )
		return false;

	auto hop1 = HiltiExpression(expr->Op1());
	auto hop2 = HiltiExpression(expr->Op2());

	func(expr, dst, hop1, hop2);
	return true;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AddExpr* expr)
	{
	auto result = Builder()->addTmp("add", HiltiType(expr->Type()));
	auto ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Add);
	ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Add);
	ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Add);
	ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_INTERVAL, ::hilti::instruction::time::Add);
	ok = ok || CompileOperator(expr, result, ::TYPE_INTERVAL, ::TYPE_INTERVAL, ::hilti::instruction::interval::Add);
	ok = ok || CompileOperator(expr, result, ::TYPE_STRING, ::TYPE_STRING, ::hilti::instruction::bytes::Concat);

	if ( ! ok )
		UnsupportedExpression(expr);

	return result;
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
	const ::Expr* lhs = expr->Op1();

	if ( lhs->Tag() == EXPR_REF )
		lhs = dynamic_cast<const ::RefExpr*>(lhs)->Op();

	auto rhs = HiltiExpression(expr->Op2());

	switch ( lhs->Tag() ) {
	case EXPR_NAME:
		{
		auto nexpr = lhs->AsNameExpr();
		auto id = nexpr->Id();
		auto dst = ::hilti::builder::id::create(HiltiSymbol(id));
		Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, rhs);
		return dst;
		}

	default:
		Error(::util::fmt("AssignExpr: no support for lhs of type %s yet", ::expr_name(lhs->Tag())), expr);
	}

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
	auto result = Builder()->addTmp("neg", HiltiType(expr->Type()));
	bool ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_INT, UNARY_OP_FUNC
		{
		Builder()->addInstruction(result,
					  ::hilti::instruction::integer::Sub,
					  ::hilti::builder::integer::create(0),
					  op);
		});

	ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, UNARY_OP_FUNC
		{
		Builder()->addInstruction(result,
					  ::hilti::instruction::integer::Sub,
					  ::hilti::builder::integer::create(0),
					  op);
		});

	ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, UNARY_OP_FUNC
		{
		Builder()->addInstruction(result,
					  ::hilti::instruction::double_::Sub,
					  ::hilti::builder::double_::create(0),
					  op);
		});

	if ( ! ok )
		UnsupportedExpression(expr);

	return result;
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
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto rtype = expr->Eval(0)->Type()->AsRecordType();

	std::vector<std::shared_ptr<::hilti::Expression>> fields;

	for ( int i = 0; i < rtype->NumFields(); i++ )
		fields.push_back(::hilti::builder::unset::create());

	loop_over_list(exprs, i)
		{
		assert(exprs[i]->Tag() == EXPR_FIELD_ASSIGN);
		FieldAssignExpr* field = (FieldAssignExpr*) exprs[i];

		int offset = rtype->FieldOffset(field->FieldName());
		assert(offset >= 0);

		fields[offset] = HiltiExpression(field->Op());
		}

	::hilti::builder::struct_::element_list elems;

	for ( auto f : fields )
		elems.push_back(f);

	return ::hilti::builder::struct_::create(elems);
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
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto ttype = expr->Eval(0)->Type()->AsTableType();

	std::shared_ptr<::hilti::Type> ktype;

	auto itypes = ttype->IndexTypes();

	if ( itypes->length() == 1 )
		ktype = HiltiType((*itypes)[0]);
	else
		{
		::hilti::builder::type_list types;

		loop_over_list(*itypes, i)
			types.push_back(HiltiType((*itypes)[i]));

		ktype = ::hilti::builder::tuple::type(types);
		}

	::hilti::builder::set::element_list elems;

	loop_over_list(exprs, i)
		elems.push_back(HiltiExpression(exprs[i]));

	return ::hilti::builder::set::create(ktype, elems);
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
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto ttype = expr->Eval(0)->Type()->AsTableType();
	auto vtype = HiltiType(ttype->YieldType());

	std::shared_ptr<::hilti::Type> ktype;

	auto itypes = ttype->IndexTypes();

	if ( itypes->length() == 1 )
		ktype = HiltiType((*itypes)[0]);
	else
		{
		::hilti::builder::type_list types;

		loop_over_list(*itypes, i)
			types.push_back(HiltiType((*itypes)[i]));

		ktype = ::hilti::builder::tuple::type(types);
		}

	::hilti::builder::map::element_list elems;

	loop_over_list(exprs, i)
		{
		auto e = exprs[i]->AsAssignExpr();
		auto v = HiltiExpression(e->Op2());

		std::shared_ptr<::hilti::Expression> k;

		if ( itypes->length() == 1 )
			k = HiltiExpression(e->Op1()->AsListExpr()->Exprs()[0]);
		else
			k = HiltiExpression(e->Op1());

		elems.push_back(std::make_pair(k, v));
		}

	return ::hilti::builder::map::create(ktype, vtype, elems);
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
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto vtype = expr->Eval(0)->Type()->AsVectorType();
	auto ytype = HiltiType(vtype->YieldType());

	::hilti::builder::vector::element_list elems;

	loop_over_list(exprs, i)
		elems.push_back(HiltiExpression(exprs[i]));

	return ::hilti::builder::vector::create(ytype, elems);
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
