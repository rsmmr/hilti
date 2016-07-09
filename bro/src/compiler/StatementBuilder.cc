
#include <Stmt.h>
#include <Func.h>
#include "consts.bif.h"
#undef List

#include <hilti/hilti.h>
#include <hilti/builder/builder.h>
#include <util/util.h>

#include "ModuleBuilder.h"
#include "StatementBuilder.h"

using namespace bro::hilti::compiler;

#define CANNOT_BE_REACHED \
	Error("code that cannot be reached has been reached"); \
	abort();

StatementBuilder::StatementBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

void StatementBuilder::NotSupported(const ::Expr* expr, const char* where)
	{
	Error(::util::fmt("expression %s not supported in %s", expr_name(expr->Tag()), where), expr);
	}

void StatementBuilder::NotSupported(const ::BroType* type, const char* where)
	{
	Error(::util::fmt("type %s not supported in %s", type_name(type->Tag()), where), type);
	}

void StatementBuilder::NotSupported(const ::Stmt* stmt)
	{
	Error(::util::fmt("statement %s not supported", stmt_name(stmt->Tag())), stmt);
	}

void StatementBuilder::PushFlowState(FlowState fstate, shared_ptr<::hilti::Expression> dst)
	{
	flow_stack.push_back(std::make_pair(fstate, dst));
	}

void StatementBuilder::PopFlowState()
	{
	flow_stack.pop_back();
	}

shared_ptr<::hilti::Expression> StatementBuilder::CurrentFlowState(FlowState fstate, bool optional)
	{
	for ( flow_state_list::reverse_iterator i = flow_stack.rbegin(); i != flow_stack.rend(); i++ )
		{
		if ( (*i).first == fstate )
			return (*i).second;
		}

	if ( optional )
		return nullptr;

	Error(::util::fmt("No current flow state of type %d available", (int)fstate));
	CANNOT_BE_REACHED
	}

void StatementBuilder::Compile(const Stmt* stmt)
	{
	// Add statement source to "bro" debugging stream.
	if ( stmt->Tag() != STMT_LIST && BifConst::Hilti::debug )
		{
		ODesc d;
		d.SetShort(1);
		stmt->Describe(&d);
		auto s = ::util::strreplace(d.Description(), "\n", " ");

		if ( s.size() )
				{
				Builder()->addComment(s);
				Builder()->addDebugMsg("bro", string("* " + s));
				}
		}

	switch ( stmt->Tag() ) {
	case STMT_ADD:
		Compile(static_cast<const ::AddStmt*>(stmt));
		break;

	case STMT_BREAK:
		Compile(static_cast<const ::BreakStmt*>(stmt));
		break;

	case STMT_DELETE:
		Compile(static_cast<const ::DelStmt*>(stmt));
		break;

	case STMT_EVENT:
		Compile(static_cast<const ::EventStmt*>(stmt));
		break;

	case STMT_EXPR:
		Compile(static_cast<const ::ExprStmt*>(stmt));
		break;

	case STMT_FALLTHROUGH:
		Compile(static_cast<const ::FallthroughStmt*>(stmt));
		break;

	case STMT_FOR:
		Compile(static_cast<const ::ForStmt*>(stmt));
		break;

	case STMT_IF:
		Compile(static_cast<const ::IfStmt*>(stmt));
		break;

	case STMT_INIT:
		Compile(static_cast<const ::InitStmt*>(stmt));
		break;

	case STMT_NEXT:
		Compile(static_cast<const ::NextStmt*>(stmt));
		break;

	case STMT_NULL:
		Compile(static_cast<const ::NullStmt*>(stmt));
		break;

	case STMT_PRINT:
		Compile(static_cast<const ::PrintStmt*>(stmt));
		break;

	case STMT_RETURN:
		Compile(static_cast<const ::ReturnStmt*>(stmt));
		break;

	case STMT_LIST:
		Compile(static_cast<const ::StmtList*>(stmt));
		break;

	case STMT_SWITCH:
		Compile(static_cast<const ::SwitchStmt*>(stmt));
		break;

	case STMT_WHEN:
		Compile(static_cast<const ::WhenStmt*>(stmt));
		break;

	default:
	        NotSupported(stmt);
	}
	}

void StatementBuilder::Compile(const ::AddStmt* stmt)
	{
	auto expr = stmt->StmtExpr();

	switch ( expr->Tag() ) {
	case EXPR_INDEX:
		{
		auto iexpr = dynamicCast(expr, const IndexExpr *);

		auto val = iexpr->Op1();
		auto idx = iexpr->Op2();

		if ( val->Type()->Tag() != TYPE_TABLE )
			NotSupported(val->Type(), "AddStmt/IndexExpr");

		assert(val->Type()->AsTableType()->IsSet());

		auto op1 = HiltiExpression(val);
		auto op2 = HiltiIndex(idx);

		Builder()->addInstruction(::hilti::instruction::set::Insert, op1, op2);
		return;
		}

	default:
		NotSupported(expr, "AddStmt");
	}

	CANNOT_BE_REACHED
	}

void StatementBuilder::Compile(const ::BreakStmt* stmt)
	{
	auto target = CurrentFlowState(FLOW_STATE_BREAK);
	Builder()->addInstruction(::hilti::instruction::flow::Jump, target);
	}

void StatementBuilder::Compile(const ::DelStmt* stmt)
	{
	auto expr = stmt->StmtExpr();

	switch ( expr->Tag() ) {
	case EXPR_INDEX:
		{
		auto iexpr = dynamicCast(expr, const IndexExpr *);

		auto val = iexpr->Op1();
		auto idx = iexpr->Op2();

		if ( val->Type()->Tag() != TYPE_TABLE )
			NotSupported(val->Type(), "DelStmt/IndexExpr");

		auto op1 = HiltiExpression(val);
		auto op2 = HiltiIndex(idx);

		if ( val->Type()->AsTableType()->IsSet() )
			Builder()->addInstruction(::hilti::instruction::set::Remove, op1, op2);
		else
			Builder()->addInstruction(::hilti::instruction::map::Remove, op1, op2);

		return;
		}

	case EXPR_FIELD:
		{
		auto fexpr = dynamicCast(expr, const FieldExpr *);
		auto op1 = HiltiExpression(fexpr->Op());
		auto op2 = HiltiStructField(fexpr->FieldName());
		Builder()->addInstruction(::hilti::instruction::struct_::Unset, op1, op2);
		return;
		}

	default:
		NotSupported(expr, "DelStmt");
	}

	CANNOT_BE_REACHED
	}

void StatementBuilder::Compile(const ::EventStmt* stmt)
	{
	auto ev = dynamicCast(stmt->StmtExpr(), const ::EventExpr *);
	HiltiCallFunction(ev, ev->Handler()->FType(), ev->Args(), nullptr);
	}

void StatementBuilder::Compile(const ::ExprStmt* stmt)
	{
	HiltiExpression(stmt->StmtExpr());
	}

void StatementBuilder::Compile(const ::FallthroughStmt* stmt)
	{
	NotSupported(stmt);
	}

void StatementBuilder::Compile(const ::ForStmt* stmt)
	{
	auto type = stmt->LoopExpr()->Type();

	switch ( type->Tag() )
		{
		case TYPE_TABLE:
			CompileForStmtTable(stmt);
			break;

		case TYPE_VECTOR:
			CompileForStmtVector(stmt);
			break;

		default:
			NotSupported(stmt);
		}
	}

void StatementBuilder::CompileForStmtTable(const ::ForStmt* stmt)
	{
	auto type = stmt->LoopExpr()->Type()->AsTableType();
	auto expr = HiltiExpression(stmt->LoopExpr());
	auto n = ::util::fmt("__i_%p", stmt);
	auto id = ::hilti::builder::id::node(n);
	shared_ptr<::hilti::Expression> iter = ::hilti::builder::id::create(n);

	std::shared_ptr<::hilti::ID> n2(nullptr);

	ModuleBuilder()->pushBody(true);
	ModuleBuilder()->pushBuilder(n2);

	// For maps, HILTI gives us a tuple of key and value during
	// iteration, while Bro expects only the former; so get that first.
	//
	// TODO: It's a pity that we throw away the value here because very
	// likely the body will have a lookup t[key] to get exactly that. We
	// could add an optimization pass at the HILTI level that would later
	// recognize this situation and reuse the value directly, omitting
	// the unnecssary lookup.
	if ( ! type->IsSet() )
		{
		auto rtype = ::ast::checkedCast<::hilti::type::Reference>(HiltiType(type));
		auto mtype = ::ast::checkedCast<::hilti::type::Map>(rtype->argType());
		auto key = Builder()->addTmp("key", mtype->keyType());
		Builder()->addInstruction(key,
					  ::hilti::instruction::tuple::Index,
					  iter,
					  ::hilti::builder::integer::create(0));

		iter = key;
		}

	auto vars = stmt->LoopVar();
	assert(vars->length());

	if ( vars->length() == 1 )
		{
		// A single value, just assign to the right local variable.
		auto id = (*vars)[0];
		auto dst = ::hilti::builder::id::create(HiltiSymbol(id));
		Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, iter);
		DeclareLocal(id);
		}

	else
		{
		// A tuple, assign each element to its corresponding variable.
		loop_over_list(*vars, i)
			{
			auto id = (*vars)[i];
			auto dst = ::hilti::builder::id::create(HiltiSymbol(id));
			Builder()->addInstruction(dst,
						  ::hilti::instruction::tuple::Index,
						  iter,
						  ::hilti::builder::integer::create(i));
			DeclareLocal(id);
			}
		}

	PushFlowState(FLOW_STATE_NEXT, ::hilti::builder::loop::next());
	PushFlowState(FLOW_STATE_BREAK, ::hilti::builder::loop::break_());

	Compile(stmt->LoopBody());

	PopFlowState();
	PopFlowState();

	ModuleBuilder()->popBuilder();
	auto body = ModuleBuilder()->popBody();

	auto body_stmt = ::ast::checkedCast<statement::Block>(body->block());
	auto s = ::hilti::builder::loop::foreach(id, expr, body_stmt);
	Builder()->addInstruction(s);
	}

void StatementBuilder::CompileForStmtVector(const ::ForStmt* stmt)
	{
	auto vexpr = HiltiExpression(stmt->LoopExpr());
	auto vars = stmt->LoopVar();
	assert(vars->length() == 1);

	auto id = new ::ID((*vars)[0]->Name(), ::SCOPE_FUNCTION, false);
	id->SetType(::base_type(TYPE_COUNT));
	auto iter = DeclareLocal(id);

	auto size = Builder()->addTmp("vsize", ::hilti::builder::integer::type(64));
	auto done = Builder()->addTmp("vdone", ::hilti::builder::boolean::type());
	Builder()->addInstruction(size, ::hilti::instruction::vector::Size, vexpr);
	Builder()->addInstruction(iter, ::hilti::instruction::operator_::Assign, ::hilti::builder::integer::create(0));

	auto next = ModuleBuilder()->newBuilder("next");
	auto cond = ModuleBuilder()->newBuilder("cond");
	Builder()->addInstruction(::hilti::instruction::flow::Jump, cond->block());

	ModuleBuilder()->pushBuilder(cond);
	Builder()->addInstruction(done, ::hilti::instruction::operator_::Equal, iter, size);
	auto b = Builder()->addIfElse(done);
	ModuleBuilder()->popBuilder(cond);

	auto btrue = std::get<0>(b);
	auto bfalse = std::get<1>(b);
	auto bcont = std::get<2>(b);

	ModuleBuilder()->pushBuilder(btrue);
	Builder()->addInstruction(::hilti::instruction::flow::Jump, bcont->block());
	ModuleBuilder()->popBuilder(btrue);

	ModuleBuilder()->pushBuilder(bfalse);

	PushFlowState(FLOW_STATE_NEXT, next->block());
	PushFlowState(FLOW_STATE_BREAK, bcont->block());
	Compile(stmt->LoopBody());
	PopFlowState();
	PopFlowState();

	Builder()->addInstruction(::hilti::instruction::flow::Jump, next->block());
	ModuleBuilder()->popBuilder(bfalse);

	ModuleBuilder()->pushBuilder(next);
	Builder()->addInstruction(iter, ::hilti::instruction::operator_::Incr, iter);
	Builder()->addInstruction(::hilti::instruction::flow::Jump, cond->block());
	ModuleBuilder()->popBuilder(next);

	ModuleBuilder()->pushBuilder(bcont);

	// Leave on stack.
	}

void StatementBuilder::Compile(const ::IfStmt* stmt)
	{
	auto cond = HiltiExpression(stmt->StmtExpr());

	if ( stmt->FalseBranch() )
		{
		auto b = Builder()->addIfElse(cond);
		auto btrue = std::get<0>(b);
		auto bfalse = std::get<1>(b);
		auto bcont = std::get<2>(b);

		ModuleBuilder()->pushBuilder(btrue);
		Compile(stmt->TrueBranch());
		Builder()->addInstruction(::hilti::instruction::flow::Jump, bcont->block());
		ModuleBuilder()->popBuilder(btrue);

		ModuleBuilder()->pushBuilder(bfalse);
		Compile(stmt->FalseBranch());
		Builder()->addInstruction(::hilti::instruction::flow::Jump, bcont->block());
		ModuleBuilder()->popBuilder(bfalse);

		ModuleBuilder()->pushBuilder(bcont);
		}

	else
		{
		auto b = Builder()->addIf(cond);
		auto btrue = std::get<0>(b);
		auto bcont = std::get<1>(b);

		ModuleBuilder()->pushBuilder(btrue);
		Compile(stmt->TrueBranch());
		Builder()->addInstruction(::hilti::instruction::flow::Jump, bcont->block());
		ModuleBuilder()->popBuilder(btrue);

		ModuleBuilder()->pushBuilder(bcont);
		}
	}

void StatementBuilder::Compile(const ::InitStmt* stmt)
	{
	// Nothing to do, we add locals on demand as they are referenced.
	}

void StatementBuilder::Compile(const ::NextStmt* stmt)
	{
	auto target = CurrentFlowState(FLOW_STATE_NEXT);
	Builder()->addInstruction(::hilti::instruction::flow::Jump, target);
	}

void StatementBuilder::Compile(const ::NullStmt* stmt)
	{
        // This one is easy. :)
	}

void StatementBuilder::Compile(const ::PrintStmt* stmt)
	{
	auto print = ::hilti::builder::id::create("Hilti::print");

	const expr_list& exprs = stmt->ExprList()->Exprs();
	auto size = exprs.length();

	if ( size == 0 )
		{
		// Just print an empty line.
		auto t = ::hilti::builder::tuple::create({ ::hilti::builder::string::create("") });
		Builder()->addInstruction(::hilti::instruction::flow::CallVoid, print, t);
		return;
		}

	loop_over_list(exprs, i)
		{
		auto bexpr = exprs[i];

		if ( i == 0 && bexpr->Type()->Tag() == TYPE_FILE )
			NotSupported(bexpr->Type(), "PrintStmt");

		if ( i > 0 )
			{
			auto op = ::hilti::builder::string::create(" ");
			auto t = ::hilti::builder::tuple::create({ op, ::hilti::builder::boolean::create(false) });
			Builder()->addInstruction(::hilti::instruction::flow::CallVoid, print, t);
			}

		auto hexpr = HiltiExpression(bexpr);

		if ( i < (size - 1) )
			{
			auto t = ::hilti::builder::tuple::create({ hexpr, ::hilti::builder::boolean::create(false) });
			Builder()->addInstruction(::hilti::instruction::flow::CallVoid, print, t);
			}

		else
			{
			auto t = ::hilti::builder::tuple::create({ hexpr });
			Builder()->addInstruction(::hilti::instruction::flow::CallVoid, print, t);
			}

		// TODO: Support printing to files.
		// TODO: Raise print_hook.
		// TODO: Support &raw_output.
		}
	}

void StatementBuilder::Compile(const ::ReturnStmt* stmt)
	{
	auto return_handler = CurrentFlowState(FLOW_STATE_RETURN, true);

	if ( return_handler )
		Builder()->addInstruction(::hilti::instruction::flow::Jump, return_handler);

	auto expr = stmt->StmtExpr();

	if ( expr )
		{
		auto hexpr = HiltiExpression(expr);
		Builder()->addInstruction(::hilti::instruction::flow::ReturnResult, hexpr);
		}

	else
		Builder()->addInstruction(::hilti::instruction::flow::ReturnVoid);
	}

void StatementBuilder::Compile(const ::StmtList* stmt)
	{
	if ( BifConst::Hilti::debug )
		Builder()->debugPushIndent();

	loop_over_list(stmt->Stmts(), i)
		{
		auto s = stmt->Stmts()[i];
		Compile(s);
		}

	if ( BifConst::Hilti::debug )
		Builder()->debugPopIndent();
	}

void StatementBuilder::Compile(const ::SwitchStmt* stmt)
	{
	NotSupported(stmt);
	}

void StatementBuilder::Compile(const ::WhenStmt* stmt)
	{
	NotSupported(stmt);
	}
