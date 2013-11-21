
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

void StatementBuilder::Compile(Stmt* stmt)
	{
	switch ( stmt->Tag() ) {
	case STMT_ADD:
		Compile(static_cast<::AddStmt*>(stmt));
		break;

	case STMT_BREAK:
		Compile(static_cast<::BreakStmt*>(stmt));
		break;

	case STMT_DELETE:
		Compile(static_cast<::DelStmt*>(stmt));
		break;

	case STMT_EVENT:
		Compile(static_cast<::EventStmt*>(stmt));
		break;

	case STMT_EXPR:
		Compile(static_cast<::ExprStmt*>(stmt));
		break;

	case STMT_FALLTHROUGH:
		Compile(static_cast<::FallthroughStmt*>(stmt));
		break;

	case STMT_FOR:
		Compile(static_cast<::ForStmt*>(stmt));
		break;

	case STMT_IF:
		Compile(static_cast<::IfStmt*>(stmt));
		break;

	case STMT_INIT:
		Compile(static_cast<::InitStmt*>(stmt));
		break;

	case STMT_NEXT:
		Compile(static_cast<::NextStmt*>(stmt));
		break;

	case STMT_NULL:
		Compile(static_cast<::NullStmt*>(stmt));
		break;

	case STMT_PRINT:
		Compile(static_cast<::PrintStmt*>(stmt));
		break;

	case STMT_RETURN:
		Compile(static_cast<::ReturnStmt*>(stmt));
		break;

	case STMT_LIST:
		Compile(static_cast<::StmtList*>(stmt));
		break;

	case STMT_SWITCH:
		Compile(static_cast<::SwitchStmt*>(stmt));
		break;

	case STMT_WHEN:
		Compile(static_cast<::WhenStmt*>(stmt));
		break;

	default:
		Error(::util::fmt("unsupported statement %s", ::stmt_name(stmt->Tag())));
	}
	}

void StatementBuilder::Compile(::AddStmt* stmt)
	{
	Error("no support yet for compiling AddStmt", stmt);
	}

void StatementBuilder::Compile(::BreakStmt* stmt)
	{
	Error("no support yet for compiling BreakStmt", stmt);
	}

void StatementBuilder::Compile(::DelStmt* stmt)
	{
	Error("no support yet for compiling DelStmt", stmt);
	}

void StatementBuilder::Compile(::EventStmt* stmt)
	{
	Error("no support yet for compiling EventStmt", stmt);
	}

void StatementBuilder::Compile(::ExprStmt* stmt)
	{
	HiltiExpression(stmt->StmtExpr());
	}

void StatementBuilder::Compile(::FallthroughStmt* stmt)
	{
	Error("no support yet for compiling FallthroughStmt", stmt);
	}

void StatementBuilder::Compile(::ForStmt* stmt)
	{
	Error("no support yet for compiling ForStmt", stmt);
	}

void StatementBuilder::Compile(::IfStmt* stmt)
	{
	Error("no support yet for compiling IfStmt", stmt);
	}

void StatementBuilder::Compile(::InitStmt* stmt)
	{
	// We don't need this, it's for initialized function parameters.
	// TODO: Is that indeed all it's used for?
	}

void StatementBuilder::Compile(::NextStmt* stmt)
	{
	Error("no support yet for compiling NextStmt", stmt);
	}

void StatementBuilder::Compile(::NullStmt* stmt)
	{
        // This one is easy. :)
	}

void StatementBuilder::Compile(::PrintStmt* stmt)
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

void StatementBuilder::Compile(::ReturnStmt* stmt)
	{
	Error("no support yet for compiling ReturnStmt", stmt);
	}

void StatementBuilder::Compile(::StmtList* stmt)
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

void StatementBuilder::Compile(::SwitchStmt* stmt)
	{
	Error("no support yet for compiling SwitchStmt", stmt);
	}

void StatementBuilder::Compile(::WhenStmt* stmt)
	{
	Error("no support yet for compiling WhenStmt", stmt);
	}
