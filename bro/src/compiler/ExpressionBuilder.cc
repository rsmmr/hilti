
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

#define CANNOT_BE_REACHED \
   	Error("code that cannot be reached has been reached"); \
	abort();

ExpressionBuilder::ExpressionBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::NotSupported(const ::BroType* type, const char* where)
	{
	Error(::util::fmt("type %s not supported in %s", type_name(type->Tag()), where), type);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::NotSupported(const ::Expr* expr, const char* where)
	{
	Error(::util::fmt("expression %s not supported in %s", expr_name(expr->Tag()), where), expr);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::NotSupported(const ::UnaryExpr* expr, const char* where)
	{
	Error(::util::fmt("unary expression %s not supported for operand of type %s",
			  ::expr_name(expr->Tag()),
			  ::type_name(expr->Op()->Type()->Tag())),
	     expr);

	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::NotSupported(const ::BinaryExpr* expr, const char* where)
	{
	Error(::util::fmt("binary expression %s not supported for operands of types %s and %s",
			  ::expr_name(expr->Tag()),
			  ::type_name(expr->Op1()->Type()->Tag()),
			  ::type_name(expr->Op2()->Type()->Tag())),
	     expr);

	return nullptr;
	}

shared_ptr<hilti::Expression> ExpressionBuilder::Compile(const ::Expr* expr, shared_ptr<::hilti::Type> target_type)
	{
	shared_ptr<::hilti::Expression> e = nullptr;

	target_types.push_back(target_type);

	switch ( expr->Tag() ) {
	case EXPR_ADD:
		e = Compile(static_cast<const ::AddExpr*>(expr));
		break;

	case EXPR_ADD_TO:
		e = Compile(static_cast<const ::AddToExpr*>(expr));
		break;

	case EXPR_ARITH_COERCE:
		e = Compile(static_cast<const ::ArithCoerceExpr*>(expr));
		break;

	case EXPR_ASSIGN:
		e = Compile(static_cast<const ::AssignExpr*>(expr));
		break;

	case EXPR_CALL:
		e = Compile(static_cast<const ::CallExpr*>(expr));
		break;

	case EXPR_CLONE:
		e = Compile(static_cast<const ::CloneExpr*>(expr));

	case EXPR_COND:
		e = Compile(static_cast<const ::CondExpr*>(expr));
		break;

	case EXPR_CONST:
		e = Compile(static_cast<const ::ConstExpr*>(expr));
		break;

	case EXPR_DIVIDE:
		e = Compile(static_cast<const ::DivideExpr*>(expr));
		break;

	case EXPR_EVENT:
		e = Compile(static_cast<const ::EventExpr*>(expr));
		break;

	case EXPR_FIELD:
		e = Compile(static_cast<const ::FieldExpr*>(expr));
		break;

	case EXPR_FIELD_ASSIGN:
		e = Compile(static_cast<const ::FieldAssignExpr*>(expr));
		break;

	case EXPR_FLATTEN:
		e = Compile(static_cast<const ::FlattenExpr*>(expr));
		break;

	case EXPR_HAS_FIELD:
		e = Compile(static_cast<const ::HasFieldExpr*>(expr));
		break;

	case EXPR_IN:
		e = Compile(static_cast<const ::InExpr*>(expr));
		break;

	case EXPR_INDEX:
		e = Compile(static_cast<const ::IndexExpr*>(expr));
		break;

	case EXPR_LIST:
		e = Compile(static_cast<const ::ListExpr*>(expr));
		break;

	case EXPR_MOD:
		e = Compile(static_cast<const ::ModExpr*>(expr));
		break;

	case EXPR_NAME:
		e = Compile(static_cast<const ::NameExpr*>(expr));
		break;

	case EXPR_NEGATE:
		e = Compile(static_cast<const ::NegExpr*>(expr));
		break;

	case EXPR_NOT:
		e = Compile(static_cast<const ::NotExpr*>(expr));
		break;

	case EXPR_POSITIVE:
		e = Compile(static_cast<const ::PosExpr*>(expr));
		break;

	case EXPR_RECORD_COERCE:
		e = Compile(static_cast<const ::RecordCoerceExpr*>(expr));
		break;

	case EXPR_RECORD_CONSTRUCTOR:
		e = Compile(static_cast<const ::RecordConstructorExpr*>(expr));
		break;

	case EXPR_REF:
		e = Compile(static_cast<const ::RefExpr*>(expr));
		break;

	case EXPR_REMOVE_FROM:
		e = Compile(static_cast<const ::RemoveFromExpr*>(expr));
		break;

	case EXPR_SCHEDULE:
		e = Compile(static_cast<const ::ScheduleExpr*>(expr));
		break;

	case EXPR_SET_CONSTRUCTOR:
		e = Compile(static_cast<const ::SetConstructorExpr*>(expr));
		break;

	case EXPR_SIZE:
		e = Compile(static_cast<const ::SizeExpr*>(expr));
		break;

	case EXPR_SUB:
		e = Compile(static_cast<const ::SubExpr*>(expr));
		break;

	case EXPR_TABLE_COERCE:
		e = Compile(static_cast<const ::TableCoerceExpr*>(expr));
		break;

	case EXPR_TABLE_CONSTRUCTOR:
		e = Compile(static_cast<const ::TableConstructorExpr*>(expr));
		break;

	case EXPR_TIMES:
		e = Compile(static_cast<const ::TimesExpr*>(expr));
		break;

	case EXPR_VECTOR_COERCE:
		e = Compile(static_cast<const ::VectorCoerceExpr*>(expr));
		break;

	case EXPR_VECTOR_CONSTRUCTOR:
		e = Compile(static_cast<const ::VectorConstructorExpr*>(expr));
		break;

	case EXPR_DECR:
	case EXPR_INCR:
		e = Compile(static_cast<const ::IncrExpr*>(expr));
		break;

	case EXPR_AND:
	case EXPR_OR:
		e = Compile(static_cast<const ::BoolExpr*>(expr));
		break;

	case EXPR_EQ:
	case EXPR_NE:
		e = Compile(static_cast<const ::EqExpr*>(expr));
		break;

	case EXPR_GE:
	case EXPR_GT:
	case EXPR_LE:
	case EXPR_LT:
		e = Compile(static_cast<const ::RelExpr*>(expr));
		break;

	default:
		NotSupported(expr, "Compile");
	}

	target_types.pop_back();
	return e;
	}

shared_ptr<::hilti::Type> ExpressionBuilder::TargetType() const
	{
	if ( target_types.size() )
		return target_types.back();

	return nullptr;
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
		NotSupported(expr, "AddExpr");

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AddToExpr* expr)
	{
	return NotSupported(expr, "AddToExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ArithCoerceExpr* expr)
	{
	auto htype = HiltiType(expr->Type());
	auto hexpr = HiltiExpression(expr->Op());

	if ( hexpr->canCoerceTo(htype) )
		return hexpr;

	Error(::util::fmt("no support in ArithCoerceExpr for coercion from %s to %s",
			  ::type_name(expr->Op()->Type()->Tag()), ::type_name(expr->Type()->Tag())), expr);

	return nullptr;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AssignExpr* expr)
	{
	return CompileAssign(expr->Op1(), expr->Op2());
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::Expr* lhs, const ::Expr* rhs)
	{
	switch ( lhs->Tag() ) {
	case EXPR_NAME:
		return CompileAssign(lhs->AsNameExpr(), rhs);

        case EXPR_REF:
		return CompileAssign(dynamic_cast<const ::RefExpr *>(lhs), rhs);

	case EXPR_INDEX:
		return CompileAssign(dynamic_cast<const ::IndexExpr *>(lhs), rhs);

	case EXPR_FIELD:
		return CompileAssign(dynamic_cast<const ::FieldExpr *>(lhs), rhs);

	case EXPR_LIST:
		return CompileAssign(lhs->AsListExpr(), rhs);

	default:
		return NotSupported(lhs, "CompileAssign");
	}

	CANNOT_BE_REACHED
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::RefExpr* lhs, const ::Expr* rhs)
	{
	return CompileAssign(lhs->Op(), rhs);
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::NameExpr* lhs, const ::Expr* rhs)
	{
	auto dst = ::hilti::builder::id::create(HiltiSymbol(lhs->Id()));
	Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, HiltiExpression(rhs));
	return dst;
        }

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::IndexExpr* lhs, const ::Expr* rhs)
	{
	switch ( lhs->Op1()->Type()->Tag() ) {
	case TYPE_TABLE:
		{
		auto tbl = HiltiExpression(lhs->Op1());
		auto idx = HiltiIndex(lhs->Op2());
		auto val = HiltiExpression(rhs);
		Builder()->addInstruction(::hilti::instruction::map::Insert, tbl, idx, val);
		return val;
		}

	case TYPE_VECTOR:
		{
		auto vec = HiltiExpression(lhs->Op1());
		auto idx = HiltiIndex(lhs->Op2());
		auto val = HiltiExpression(rhs);
		Builder()->addInstruction(::hilti::instruction::vector::Set, vec, idx, val);
		return val;
		}

	default:
		return NotSupported(lhs->Type(), "CompileAssign/IndexExpr");
	}

	CANNOT_BE_REACHED
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::FieldExpr* lhs, const ::Expr* rhs)
	{
	auto op1 = HiltiExpression(lhs->Op());
	auto op2 = HiltiStructField(lhs->FieldName());
	auto op3 = HiltiExpression(rhs);
	Builder()->addInstruction(::hilti::instruction::struct_::Set, op1, op2, op3);
	return op3;
        }

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::ListExpr* lhs, const ::Expr* rhs)
	{
	return NotSupported(lhs, "CompileAssign/ListExpr");
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
	return NotSupported(expr, "CloneExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CondExpr* expr)
	{
	return NotSupported(expr, "CondExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ConstExpr* expr)
	{
	return HiltiValue(expr->Value());
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::DivideExpr* expr)
	{
	return NotSupported(expr, "DivideExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::EventExpr* expr)
	{
	return NotSupported(expr, "EventExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::FieldExpr* expr)
	{
	auto rtype = expr->Op()->Type()->AsRecordType();
	auto result = Builder()->addTmp("field", HiltiType(rtype->FieldType(expr->Field())));

	auto op1 = HiltiExpression(expr->Op());
	auto op2 = HiltiStructField(expr->FieldName());
	Builder()->addInstruction(result, ::hilti::instruction::struct_::Get, op1, op2);

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::FieldAssignExpr* expr)
	{
	return NotSupported(expr, "FieldAssignExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::FlattenExpr* expr)
	{
	return NotSupported(expr, "FlattenExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::HasFieldExpr* expr)
	{
	auto result = Builder()->addTmp("has_field", ::hilti::builder::boolean::type());
	auto op1 = HiltiExpression(expr->Op());
	auto op2 = HiltiStructField(expr->FieldName());
	Builder()->addInstruction(result, ::hilti::instruction::struct_::IsSet, op1, op2);
	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::InExpr* expr)
	{
	return NotSupported(expr, "InExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::IndexExpr* expr)
	{
	auto ty = expr->Op1()->Type();

	switch ( ty->Tag() ) {
	case TYPE_TABLE:
		{
		auto mtype = ty->AsTableType();
		auto result = Builder()->addTmp("yield", HiltiType(mtype->YieldType()));
		auto op1 = HiltiExpression(expr->Op1());
                auto op2 = HiltiIndex(expr->Op2());
		Builder()->addInstruction(result, ::hilti::instruction::map::Get, op1, op2);
		return result;
		}

	case TYPE_VECTOR:
		{
		// TODO: No support for vector indices yet.
		auto vtype = ty->AsVectorType();
		auto result = Builder()->addTmp("yield", HiltiType(vtype->YieldType()));
		auto op1 = HiltiExpression(expr->Op1());
		auto op2 = HiltiExpression(expr->Op2());

		auto idx = Builder()->addTmp("idx", ::hilti::builder::integer::type(64));

		Builder()->addInstruction(idx,
					  ::hilti::instruction::tuple::Index,
					  op2,
					  ::hilti::builder::integer::create(0));

		Builder()->addInstruction(result, ::hilti::instruction::vector::Get, op1, idx);
		return result;
		}

	case TYPE_STRING:
		{
		auto result = Builder()->addTmp("substr", HiltiType(ty));
		auto op1 = HiltiExpression(expr->Op1());
		auto op2 = HiltiExpression(expr->Op2());

		auto offset = Builder()->addTmp("offset", ::hilti::builder::integer::type(64));
		auto i = Builder()->addTmp("i", ::hilti::builder::iterator::typeBytes());
		auto j = Builder()->addTmp("j", ::hilti::builder::iterator::typeBytes());

		// HILTI handles negative offsets correctly.

		Builder()->addInstruction(offset,
					  ::hilti::instruction::tuple::Index,
					  op2,
					  ::hilti::builder::integer::create(0));

		Builder()->addInstruction(i, ::hilti::instruction::bytes::Offset, op1, offset);

		if ( expr->Op2()->Type()->AsTypeList()->Types()->length() == 1 )
			Builder()->addInstruction(j, ::hilti::instruction::operator_::Assign, i);

		else
			{
			Builder()->addInstruction(offset,
						  ::hilti::instruction::tuple::Index,
						  op2,
						  ::hilti::builder::integer::create(1));
			Builder()->addInstruction(j, ::hilti::instruction::bytes::Offset, op1, offset);
			}

		// Bro's end index is including.
		Builder()->addInstruction(j, ::hilti::instruction::operator_::Incr, j);

		Builder()->addInstruction(result, ::hilti::instruction::bytes::Sub, i, j);
		return result;
		}

	default:
		return NotSupported(ty, "IndexExpr");
	}

	CANNOT_BE_REACHED
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
	return NotSupported(expr, "ModExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::NameExpr* expr)
	{
	auto id = expr->Id();

	if ( id->IsGlobal() && (id->IsConst() || id->Type()->Tag() == TYPE_FUNC) )
		{
		// TODO: This is technically not correct for global
		// functions; they aren't constants. If somebody reassigs a
		// global function this will not account for that.
		auto val = id->ID_Val();
		assert(val);
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
		NotSupported(expr, "NegExpr");

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::NotExpr* expr)
	{
	return NotSupported(expr, "NotExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::PosExpr* expr)
	{
	return NotSupported(expr, "PosExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RecordCoerceExpr* expr)
	{
	return NotSupported(expr, "RecordCoerceExpr");
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
	return NotSupported(expr, "RefExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RemoveFromExpr* expr)
	{
	return NotSupported(expr, "RemoveFromExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ScheduleExpr* expr)
	{
	return NotSupported(expr, "ScheduleExpr");
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
	return NotSupported(expr, "SizeExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::SubExpr* expr)
	{
	return NotSupported(expr, "SubExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TableCoerceExpr* expr)
	{
	return NotSupported(expr, "TableCoerceExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TableConstructorExpr* expr)
	{
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto ttype = expr->Eval(0)->Type()->AsTableType();
	auto vtype = HiltiType(ttype->YieldType());

	if ( ttype->IsUnspecifiedTable() )
		{
		auto target = TargetType();

		if ( ! target )
			{
			Error("UnspecifiedTable but no target type in ExpressionBuilder::TableCtorExpr", expr);
			return 0;
			}

		return ::hilti::builder::expression::default_(target);
		}

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
	return NotSupported(expr, "TimesExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::VectorCoerceExpr* expr)
	{
	return NotSupported(expr, "VectorCoerceExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::VectorConstructorExpr* expr)
	{
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto vtype = expr->Eval(0)->Type()->AsVectorType();
	auto ytype = HiltiType(vtype->YieldType());

	if ( vtype->IsUnspecifiedVector() )
		{
		auto target = TargetType();

		if ( ! target )
			{
			Error("UnspecifiedVector but no target type in ExpressionBuilder::VectorCtorExpr", expr);
			return 0;
			}

		return ::hilti::builder::expression::default_(target);
		}

	::hilti::builder::vector::element_list elems;

	loop_over_list(exprs, i)
		elems.push_back(HiltiExpression(exprs[i]));

	return ::hilti::builder::vector::create(ytype, elems);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::IncrExpr* expr)
	{
	return NotSupported(expr, "IncrExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::BoolExpr* expr)
	{
	return NotSupported(expr, "BoolExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::EqExpr* expr)
	{
	return NotSupported(expr, "EqExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RelExpr* expr)
	{
	return NotSupported(expr, "RelExpr");
	}
