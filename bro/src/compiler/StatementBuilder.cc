
#include <Stmt.h>
#undef List

#include <hilti/hilti.h>
#include <hilti/builder/builder.h>
#include <util/util.h>

#include "ModuleBuilder.h"
#include "StatementBuilder.h"

using namespace bro::hilti::compiler;

StatementBuilder::StatementBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

void StatementBuilder::Compile(const Stmt* stmt)
	{
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
		Error(::util::fmt("unsupported statement %s", ::stmt_name(stmt->Tag())));
	}
	}

void StatementBuilder::Compile(const ::AddStmt* stmt)
	{
	Error("no support yet for compiling AddStmt", stmt);
	}

void StatementBuilder::Compile(const ::BreakStmt* stmt)
	{
	Error("no support yet for compiling BreakStmt", stmt);
	}

void StatementBuilder::Compile(const ::DelStmt* stmt)
	{
	Error("no support yet for compiling DelStmt", stmt);
	}

void StatementBuilder::Compile(const ::EventStmt* stmt)
	{
	Error("no support yet for compiling EventStmt", stmt);
	}

void StatementBuilder::Compile(const ::ExprStmt* stmt)
	{
	HiltiExpression(stmt->StmtExpr());
	}

void StatementBuilder::Compile(const ::FallthroughStmt* stmt)
	{
	Error("no support yet for compiling FallthroughStmt", stmt);
	}

void StatementBuilder::Compile(const ::ForStmt* stmt)
	{
	auto type = stmt->LoopExpr()->Type();
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
	if ( type->Tag() == TYPE_TABLE && ! type->AsTableType()->IsSet() )
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
		auto dst = ::hilti::builder::id::create(HiltiSymbol((*vars)[0]));
		Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, iter);
		}

	else
		{
		// A tuple, assign each element to its corresponding variable.
		loop_over_list(*vars, i)
			{
			auto dst = ::hilti::builder::id::create(HiltiSymbol((*vars)[i]));
			Builder()->addInstruction(dst,
						  ::hilti::instruction::tuple::Index,
						  iter,
						  ::hilti::builder::integer::create(i));
			}
		}

	Compile(stmt->LoopBody());

    ModuleBuilder()->popBuilder();
    auto body = ModuleBuilder()->popBody();

	auto body_stmt = ::ast::checkedCast<statement::Block>(body->block());
	auto s = ::hilti::builder::loop::foreach(id, expr, body_stmt);
	Builder()->addInstruction(s);
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
	// We don't need this, it's for initialized function parameters.
	// TODO: Is that indeed all it's used for?
	}

void StatementBuilder::Compile(const ::NextStmt* stmt)
	{
	Error("no support yet for compiling NextStmt", stmt);
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
			Error("Compiling PrintStmt does not yet support output to files");

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
	Error("no support yet for compiling ReturnStmt", stmt);
	}

void StatementBuilder::Compile(const ::StmtList* stmt)
	{
	loop_over_list(stmt->Stmts(), i)
		{
		auto s = stmt->Stmts()[i];

		if ( s->Tag() != STMT_LIST )
			{
			ODesc d;
			s->Describe(&d);
			string cmt = d.Description();
			cmt = ::util::strreplace(cmt, "\n", " ");
			Builder()->addComment(cmt);
			// Builder()->addDebugMsg("bro", cmt);
			}

		Compile(s);
		}
	}

void StatementBuilder::Compile(const ::SwitchStmt* stmt)
	{
	Error("no support yet for compiling SwitchStmt", stmt);
	}

void StatementBuilder::Compile(const ::WhenStmt* stmt)
	{
	Error("no support yet for compiling WhenStmt", stmt);
	}
