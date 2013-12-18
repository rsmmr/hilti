
#include <Expr.h>
#undef List

#include "ASTDumper.h"

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
	Error(::util::fmt("unary expression %s not supported for operand of type %s in %s",
			  ::expr_name(expr->Tag()),
			  ::type_name(expr->Op()->Type()->Tag()),
			  where),
	      expr);

	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::NotSupported(const ::BinaryExpr* expr, const char* where)
	{
	Error(::util::fmt("binary expression %s not supported for operands of types %s and %s in %s",
			  ::expr_name(expr->Tag()),
			  ::type_name(expr->Op1()->Type()->Tag()),
			  ::type_name(expr->Op2()->Type()->Tag()),
              where),
	     expr);

	return nullptr;
	}

shared_ptr<hilti::Expression> ExpressionBuilder::Compile(const ::Expr* expr, const ::BroType* target_type)
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

	if ( target_type && target_type->Tag() == ::TYPE_ANY )
		// Need to pass an actual Bro value.
		e = RuntimeHiltiToVal(e, expr->Type());

	return e;
	}

const ::BroType* ExpressionBuilder::TargetType() const
	{
	if ( ! target_types.size() || ! target_types.back() )
		{
		Error("ExpressionBuilder::TargetType() used, but no target type set.");
		return 0;
		}

	auto t = target_types.back();
	assert(t);
	return t;
	}

bool ExpressionBuilder::HasTargetType() const
	{
	return target_types.size() && target_types.back();
	}

bool ExpressionBuilder::CompileOperator(const ::Expr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op, shared_ptr<::hilti::Instruction> ins, shared_ptr<::hilti::Expression> addl_op)
	{
	if ( expr->Type()->Tag() != op )
		return false;

	auto hop = HiltiExpression(expr);
	Builder()->addInstruction(dst, ins, hop, addl_op);
	return true;
	}

bool ExpressionBuilder::CompileOperator(const ::UnaryExpr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op, shared_ptr<::hilti::Instruction> ins, shared_ptr<::hilti::Expression> addl_op)
	{
	if ( expr->Op()->Type()->Tag() != op )
		return false;

	auto hop = HiltiExpression(expr->Op());
	Builder()->addInstruction(dst, ins, hop, addl_op);
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

bool ExpressionBuilder::CompileOperator(const ::BinaryExpr* expr, shared_ptr<::hilti::Expression> dst, ::TypeTag op1, ::TypeTag op2, shared_ptr<::hilti::Instruction> ins, shared_ptr<::hilti::Expression> addl_op)
	{
	if ( expr->Op1()->Type()->Tag() != op1 || expr->Op2()->Type()->Tag() != op2 )
		return false;

	auto hop1 = HiltiExpression(expr->Op1());
	auto hop2 = HiltiExpression(expr->Op2());

	Builder()->addInstruction(dst, ins, hop1, hop2, addl_op);
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
		NotSupported(expr->Type(), "AddExpr");

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAddTo(const ::Expr* expr, shared_ptr<::hilti::Expression> val, bool add)
	{
	auto nval = Builder()->addTmp("addto", HiltiType(expr->Type()));
	auto ok = false;

	if ( add )
		{
		ok = ok || CompileOperator(expr, nval, ::TYPE_INT, ::hilti::instruction::integer::Add, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_COUNT, ::hilti::instruction::integer::Add, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_INTERVAL, ::hilti::instruction::interval::Add, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_TIME, ::hilti::instruction::time::Add, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_DOUBLE, ::hilti::instruction::double_::Add, val);
        ok = ok || CompileOperator(expr, nval, ::TYPE_STRING, ::hilti::instruction::bytes::Concat, val);
		}

	else
		{
		ok = ok || CompileOperator(expr, nval, ::TYPE_INT, ::hilti::instruction::integer::Sub, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_COUNT, ::hilti::instruction::integer::Sub, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_INTERVAL, ::hilti::instruction::interval::Sub, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_TIME, ::hilti::instruction::time::SubTime, val);
		ok = ok || CompileOperator(expr, nval, ::TYPE_DOUBLE, ::hilti::instruction::double_::Sub, val);
		}

	if ( ! ok )
		NotSupported(expr->Type(), "AddExpr");

	return CompileAssign(expr, nval);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::AddToExpr* expr)
	{
	return CompileAddTo(expr->Op1(), HiltiExpression(expr->Op2()), true);
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
	auto hrhs = HiltiExpression(rhs, lhs->Type());
	return CompileAssign(lhs, hrhs);
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::Expr* lhs, std::shared_ptr<::hilti::Expression> rhs)
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

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::RefExpr* lhs, std::shared_ptr<::hilti::Expression> rhs)
	{
	return CompileAssign(lhs->Op(), rhs);
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::NameExpr* lhs, std::shared_ptr<::hilti::Expression> rhs)
	{
	auto id = lhs->Id();

	if ( id->IsGlobal() && ! id->AsType() )
		DeclareGlobal(id);

	if ( ! id->IsGlobal() )
		DeclareLocal(id);

	auto dst = ::hilti::builder::id::create(HiltiSymbol(id));
	Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, rhs);

	return dst;
        }

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::IndexExpr* lhs, std::shared_ptr<::hilti::Expression> rhs)
	{
	switch ( lhs->Op1()->Type()->Tag() ) {
	case TYPE_TABLE:
		{
		auto tbl = HiltiExpression(lhs->Op1());
		auto idx = HiltiIndex(lhs->Op2());
		auto val = rhs;
		Builder()->addInstruction(::hilti::instruction::map::Insert, tbl, idx, val);
		return val;
		}

	case TYPE_VECTOR:
		{
		auto vec = HiltiExpression(lhs->Op1());
		auto idx = HiltiIndex(lhs->Op2());
		auto val = rhs;
		Builder()->addInstruction(::hilti::instruction::vector::Set, vec, idx, val);
		return val;
		}

	default:
		return NotSupported(lhs->Type(), "CompileAssign/IndexExpr");
	}

	CANNOT_BE_REACHED
	}

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::FieldExpr* lhs, std::shared_ptr<::hilti::Expression> rhs)
	{
	auto op1 = HiltiExpression(lhs->Op());
	auto op2 = HiltiStructField(lhs->FieldName());
	auto op3 = rhs;
	Builder()->addInstruction(::hilti::instruction::struct_::Set, op1, op2, op3);
	return op3;
        }

std::shared_ptr<::hilti::Expression> ExpressionBuilder::CompileAssign(const ::ListExpr* lhs, std::shared_ptr<::hilti::Expression> rhs)
	{
	return NotSupported(lhs, "CompileAssign/ListExpr");
        }

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CallExpr* expr)
	{
	return HiltiCallFunction(expr->Func(), expr->Func()->Type()->AsFuncType(), expr->Args());
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CloneExpr* expr)
	{
	return NotSupported(expr, "CloneExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::CondExpr* expr)
	{
	auto result = Builder()->addTmp("cond", HiltiType(expr->Type()));

	auto cond = HiltiExpression(expr->Op1());
	auto b = Builder()->addIfElse(cond);

	auto true_ = std::get<0>(b);
	auto false_ = std::get<1>(b);
	auto cont = std::get<2>(b);

	ModuleBuilder()->pushBuilder(true_);
	auto e = HiltiExpression(expr->Op2(), expr->Type());
	Builder()->addInstruction(result, ::hilti::instruction::operator_::Assign, e);
	Builder()->addInstruction(::hilti::instruction::flow::Jump, cont->block());
	ModuleBuilder()->popBuilder(true_);

	ModuleBuilder()->pushBuilder(false_);
	e = HiltiExpression(expr->Op3(), expr->Type());
	Builder()->addInstruction(result, ::hilti::instruction::operator_::Assign, e);
	Builder()->addInstruction(::hilti::instruction::flow::Jump, cont->block());
	ModuleBuilder()->popBuilder(false_);

	ModuleBuilder()->pushBuilder(cont);
	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ConstExpr* expr)
	{
	return HiltiValue(expr->Value());
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::DivideExpr* expr)
	{
	auto result = Builder()->addTmp("div", HiltiType(expr->Type()));
	auto ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Div);
	ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Div);
	ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Div);

	if ( ! ok )
		NotSupported(expr, "DivideExpr");

	return result;
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
	auto result = Builder()->addTmp("has", ::hilti::builder::boolean::type());

	// Patterns are on the LHS.
	if ( expr->Op1()->Type()->Tag() == TYPE_PATTERN )
		{
		auto op1 = HiltiExpression(expr->Op1());
		auto op2 = HiltiExpression(expr->Op2());

		auto begin = Builder()->addTmp("begin", ::hilti::builder::iterator::typeBytes());
		Builder()->addInstruction(begin, ::hilti::instruction::operator_::Begin, op2);

		auto pos = Builder()->addTmp("pos", ::hilti::builder::integer::type(32));
		Builder()->addInstruction(pos, ::hilti::instruction::regexp::FindBytes, op1, begin);

		auto zero = ::hilti::builder::integer::create(0);
		Builder()->addInstruction(result, ::hilti::instruction::integer::Sgeq, pos, zero);

		return result;
		}

	auto ty = expr->Op2()->Type();

	switch ( ty->Tag() ) {
	case TYPE_TABLE:
		{
		auto mtype = ty->AsTableType();
		auto op1 = HiltiExpression(expr->Op2());
		auto op2 = HiltiIndex(expr->Op1());

		if ( ty->IsSet() )
			Builder()->addInstruction(result, ::hilti::instruction::set::Exists, op1, op2);
		else
			Builder()->addInstruction(result, ::hilti::instruction::map::Exists, op1, op2);

		return result;
		}

	case TYPE_STRING:
		{
		auto op1 = HiltiExpression(expr->Op2());
		auto op2 = HiltiExpression(expr->Op1());
		Builder()->addInstruction(result, ::hilti::instruction::bytes::Contains, op1, op2);
		return result;
		}

	case TYPE_SUBNET:
		{
		auto op1 = HiltiExpression(expr->Op2());
		auto op2 = HiltiExpression(expr->Op1());
		Builder()->addInstruction(result, ::hilti::instruction::network::Contains, op1, op2);
		return result;
		}

	default:
		return NotSupported(ty, "InExpr");
	}

	CANNOT_BE_REACHED
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

	auto targets = HasTargetType() ? TargetType()->AsTypeList()->Types() : nullptr;

	loop_over_list(exprs, i)
		{
		auto hexpr = HiltiExpression(exprs[i], targets ? (*targets)[i] : nullptr);
		elems.push_back(hexpr);
		}

	return ::hilti::builder::tuple::create(elems);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ModExpr* expr)
	{
	auto result = Builder()->addTmp("mod", HiltiType(expr->Type()));
	auto ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Mod);
	ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Mod);
	ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Mod);

	if ( ! ok )
		NotSupported(expr, "ModExpr");

	return result;
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

	if ( id->AsType() )
		return HiltiBroVal(id->Type());

	if ( id->IsGlobal() )
		DeclareGlobal(id);
	else
		DeclareLocal(id);

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
	auto result = Builder()->addTmp("not", ::hilti::builder::boolean::type());
	auto op1 = HiltiExpression(expr->Op());
	Builder()->addInstruction(result, ::hilti::instruction::boolean::Not, op1);
	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::PosExpr* expr)
	{
	return NotSupported(expr, "PosExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::CompileListExprToRecordTuple(const ::ListExpr* lexpr, const ::RecordType* rtype)
	{
	const expr_list& exprs = lexpr->Exprs();

	std::vector<std::shared_ptr<::hilti::Expression>> fields;

	for ( int i = 0; i < rtype->NumFields(); i++ )
		fields.push_back(::hilti::builder::unset::create());

	loop_over_list(exprs, i)
		{
		assert(exprs[i]->Tag() == EXPR_FIELD_ASSIGN);
		FieldAssignExpr* field = (FieldAssignExpr*) exprs[i];

		int offset = rtype->FieldOffset(field->FieldName());
		assert(offset >= 0);

		auto ftype = rtype->FieldType(field->FieldName());
		fields[offset] = HiltiExpression(field->Op(), ftype);
		}

	::hilti::builder::struct_::element_list elems;

	for ( auto f : fields )
		elems.push_back(f);

	return ::hilti::builder::struct_::create(elems);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RecordCoerceExpr* expr)
	{
	auto super_rtype = expr->Type()->AsRecordType();
	auto sub_rtype = expr->Op()->Type()->AsRecordType();

	// We optimize here for the common case that the rhs is a record ctor.
	if ( expr->Op()->Tag() == EXPR_RECORD_CONSTRUCTOR )
		{
		auto rctor = dynamic_cast<::RecordConstructorExpr *>(expr->Op());
		auto tuple = CompileListExprToRecordTuple(rctor->Op()->AsListExpr(), super_rtype);
		return tuple;
		}

	std::vector<std::shared_ptr<::hilti::Expression>> fields;

	for ( int i = 0; i < super_rtype->NumFields(); i++ )
		fields.push_back(::hilti::builder::unset::create());

	auto op1 = HiltiExpression(expr->Op());

	for ( int i = 0; i < sub_rtype->NumFields(); i++ )
		{
		auto fname = sub_rtype->FieldName(i);
		auto ftype = sub_rtype->FieldType(i);

		auto ftmp = ModuleBuilder()->addTmp("f", HiltiType(ftype));
		auto op2 = HiltiStructField(fname);
		Builder()->addInstruction(ftmp, ::hilti::instruction::struct_::Get, op1, op2);

		int offset = super_rtype->FieldOffset(fname);
		assert(offset >= 0);

		fields[offset] = ftmp;
		}

	::hilti::builder::struct_::element_list elems;

	for ( auto f : fields )
		elems.push_back(f);

	auto tuple = ::hilti::builder::struct_::create(elems);
	auto rec = ModuleBuilder()->addTmp("rec", HiltiType(expr->Type()));
	Builder()->addInstruction(rec, ::hilti::instruction::operator_::Assign, tuple);
	return rec;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RecordConstructorExpr* expr)
	{
	auto rtype = HasTargetType() ? TargetType() : expr->Type();
	auto tuple = CompileListExprToRecordTuple(expr->Op()->AsListExpr(), rtype->AsRecordType());

	auto rec = ModuleBuilder()->addTmp("rec", HiltiType(rtype));
	Builder()->addInstruction(rec, ::hilti::instruction::operator_::Assign, tuple);

	return rec;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RefExpr* expr)
	{
	return Compile(expr->Op());
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RemoveFromExpr* expr)
	{
	return CompileAddTo(expr->Op1(), HiltiExpression(expr->Op2()), false);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::ScheduleExpr* expr)
	{
	return NotSupported(expr, "ScheduleExpr");
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::SetConstructorExpr* expr)
	{
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto ttype = expr->Type()->AsTableType();

	if ( ttype->IsUnspecifiedTable() )
		return ::hilti::builder::expression::default_(HiltiType(TargetType()));

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
	auto result = Builder()->addTmp("sub", ::hilti::builder::integer::type(64));
	auto ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_STRING, ::hilti::instruction::bytes::Length);
	ok = ok || CompileOperator(expr, result, ::TYPE_VECTOR, ::hilti::instruction::vector::Size);

	ok = ok || CompileOperator(expr, result, ::TYPE_TABLE, UNARY_OP_FUNC
		{
		if ( expr->Op()->Type()->AsTableType()->IsSet() )
			Builder()->addInstruction(result,
						  ::hilti::instruction::set::Size,
						  op);
		else
			Builder()->addInstruction(result,
						  ::hilti::instruction::map::Size,
						  op);
		});

	ok = ok || CompileOperator(expr, result, ::TYPE_LIST, UNARY_OP_FUNC
		{
		int size = expr->Op()->Type()->AsTypeList()->Types()->length();

		Builder()->addInstruction(result,
					  ::hilti::instruction::operator_::Assign,
					  ::hilti::builder::integer::create(size));
		});

	if ( ! ok )
		NotSupported(expr, "SubExpr");

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::SubExpr* expr)
	{
	auto result = Builder()->addTmp("sub", HiltiType(expr->Type()));
	auto ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Sub);
	ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Sub);
	ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Sub);
	ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_INTERVAL, ::hilti::instruction::time::SubInterval);
	ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_TIME, ::hilti::instruction::time::SubTime);
	ok = ok || CompileOperator(expr, result, ::TYPE_INTERVAL, ::TYPE_INTERVAL, ::hilti::instruction::interval::Sub);

	if ( ! ok )
		NotSupported(expr, "SubExpr");

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TableCoerceExpr* expr)
	{
	auto t = HiltiType(expr->Type());
	return ::hilti::builder::expression::default_(t);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::TableConstructorExpr* expr)
	{
	const expr_list& exprs = expr->Op()->AsListExpr()->Exprs();

	auto ttype = expr->Type()->AsTableType();
	auto vtype = HiltiType(ttype->YieldType());

	if ( ttype->IsUnspecifiedTable() )
		return ::hilti::builder::expression::default_(HiltiType(TargetType()));

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
	auto result = Builder()->addTmp("mult", HiltiType(expr->Type()));
	auto ok = false;

	ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Mul);
	ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Mul);
	ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Mul);

	if ( ! ok )
		NotSupported(expr, "MulExpr");

	return result;
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
		return ::hilti::builder::expression::default_(HiltiType(TargetType()));

	::hilti::builder::vector::element_list elems;

	loop_over_list(exprs, i)
		elems.push_back(HiltiExpression(exprs[i]));

	return ::hilti::builder::vector::create(ytype, elems);
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::IncrExpr* expr)
	{
	auto incr = ::hilti::builder::integer::create(1);
	return CompileAddTo(expr->Op(), incr, (expr->Tag() == EXPR_INCR));
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::BoolExpr* expr)
	{
	auto result = Builder()->addTmp("b", ::hilti::builder::boolean::type());

	bool ok = false;

	switch ( expr->Tag() ) {
	case EXPR_AND:
		{
		ok = ok || CompileOperator(expr, result, ::TYPE_BOOL, ::TYPE_BOOL, ::hilti::instruction::boolean::And);
		break;
		}

	case EXPR_OR:
		{
		ok = ok || CompileOperator(expr, result, ::TYPE_BOOL, ::TYPE_BOOL, ::hilti::instruction::boolean::Or);
		break;
		}

	default:
		return NotSupported(expr, "BoolExpr");
	}

	if ( ! ok )
		NotSupported(expr, "BoolExpr");

	return result;
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::EqExpr* expr)
	{
	auto result = Builder()->addTmp("eq", ::hilti::builder::boolean::type());
	auto op1 = HiltiExpression(expr->Op1());
	auto op2 = HiltiExpression(expr->Op2());

	if ( expr->Op1()->Type()->Tag() == TYPE_PATTERN )
		{
		// Bro canonicalizes these so that the pattern is always in op1.

		auto itype = ::hilti::builder::iterator::typeBytes();

		auto begin = Builder()->addTmp("begin", itype);
		auto end = Builder()->addTmp("end", ::hilti::builder::iterator::typeBytes());
		Builder()->addInstruction(begin, ::hilti::instruction::operator_::Begin, op2);
		Builder()->addInstruction(end, ::hilti::instruction::operator_::End, op2);

		auto ipair = ::hilti::builder::tuple::type({ itype, itype });
		auto rtype = ::hilti::builder::tuple::type({ ::hilti::builder::integer::type(32), ipair });
		auto m = Builder()->addTmp("m", rtype);
		Builder()->addInstruction(m, ::hilti::instruction::regexp::SpanBytes, op1, begin);

		auto zero = ::hilti::builder::integer::create(0);
		auto one = ::hilti::builder::integer::create(1);
		auto iters = Builder()->addTmp("iters", ipair);
		Builder()->addInstruction(iters, ::hilti::instruction::tuple::Index, m, one);

		auto i1 = Builder()->addTmp("i1", itype);
		auto i2 = Builder()->addTmp("i2", itype);
		Builder()->addInstruction(i1, ::hilti::instruction::tuple::Index, iters, zero);
		Builder()->addInstruction(i2, ::hilti::instruction::tuple::Index, iters, one);

		auto b1 = Builder()->addTmp("b1", ::hilti::builder::boolean::type());
		auto b2 = Builder()->addTmp("b2", ::hilti::builder::boolean::type());
		Builder()->addInstruction(b1, ::hilti::instruction::operator_::Equal, i1, begin);
		Builder()->addInstruction(b2, ::hilti::instruction::operator_::Equal, i2, end);

		Builder()->addInstruction(result, ::hilti::instruction::boolean::And, b1, b2);
		return result;
		}

	switch ( expr->Tag() ) {
	case EXPR_EQ:
		Builder()->addInstruction(result, ::hilti::instruction::operator_::Equal, op1, op2);
		return result;

	case EXPR_NE:
		Builder()->addInstruction(result, ::hilti::instruction::operator_::Unequal, op1, op2);
		return result;

	default:
		return NotSupported(expr, "EqExpr");
	}

	CANNOT_BE_REACHED
	}

shared_ptr<::hilti::Expression> ExpressionBuilder::Compile(const ::RelExpr* expr)
	{
	auto result = Builder()->addTmp("eq", ::hilti::builder::boolean::type());

	bool ok = false;

	switch ( expr->Tag() ) {
	case EXPR_LT:
		{
		ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Slt);
		ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Ult);
		ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Lt);
		ok = ok || CompileOperator(expr, result, ::TYPE_INTERVAL, ::TYPE_INTERVAL, ::hilti::instruction::interval::Lt);
		ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_TIME, ::hilti::instruction::time::Lt);
		break;
		}

	case EXPR_LE:
		{
		ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Sleq);
		ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Uleq);
		ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Leq);
		ok = ok || CompileOperator(expr, result, ::TYPE_INTERVAL, ::TYPE_INTERVAL, ::hilti::instruction::interval::Leq);
		ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_TIME, ::hilti::instruction::time::Leq);
		break;
		}

	case EXPR_GT:
		{
		ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Sgt);
		ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Ugt);
		ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Gt);
		ok = ok || CompileOperator(expr, result, ::TYPE_INTERVAL, ::TYPE_INTERVAL, ::hilti::instruction::interval::Gt);
		ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_TIME, ::hilti::instruction::time::Gt);
		break;
		}

	case EXPR_GE:
		{
		ok = ok || CompileOperator(expr, result, ::TYPE_INT, ::TYPE_INT, ::hilti::instruction::integer::Sgeq);
		ok = ok || CompileOperator(expr, result, ::TYPE_COUNT, ::TYPE_COUNT, ::hilti::instruction::integer::Ugeq);
		ok = ok || CompileOperator(expr, result, ::TYPE_DOUBLE, ::TYPE_DOUBLE, ::hilti::instruction::double_::Geq);
		ok = ok || CompileOperator(expr, result, ::TYPE_INTERVAL, ::TYPE_INTERVAL, ::hilti::instruction::interval::Geq);
		ok = ok || CompileOperator(expr, result, ::TYPE_TIME, ::TYPE_TIME, ::hilti::instruction::time::Geq);
		break;
		}

	default:
		return NotSupported(expr, "RelExpr");
	}

	if ( ! ok )
		NotSupported(expr, "RelExpr");

	return result;
	}
