
#include <iostream>
#include <cxxabi.h>

#include <util/util.h>

#include "ASTDumper.h"
#include "Func.h"
#include "Expr.h"
#include "Stmt.h"
#include "ID.h"
#undef List

using namespace bro::hilti::compiler;

void ASTDumper::Dump(const ::Func *func)
	{
	ASTDumper cb;
	func->Traverse(&cb);
	}

void ASTDumper::Dump(const ::Stmt *stmt)
	{
	ASTDumper cb;
	stmt->Traverse(&cb);
	}

void ASTDumper::Dump(const ::Expr *expr)
	{
	ASTDumper cb;
	expr->Traverse(&cb);
	}

void ASTDumper::Dump(const ::ID *id)
	{
	ASTDumper cb;
	id->Traverse(&cb);
	}

TraversalCode ASTDumper::DoPre(const char* tag, const ::BroObj* obj)
	{
	ODesc d;
	d.SetShort();
	obj->Describe(&d);
	string desc = ::util::strreplace(d.Description(), "\n", " / ");

	int status;
	char* cls = ::abi::__cxa_demangle(typeid(*obj).name(), 0, 0, &status);

	std::cerr << ::util::fmt("%15s | ", std::string(cls).substr(0, 15));

	free(cls);

	for ( int i = 0; i < depth; i++ )
		std::cerr << "  ";

	std::cerr << desc << std::endl;

	++depth;

	return TC_CONTINUE;
	}

TraversalCode ASTDumper::DoPost(const char* tag, const ::BroObj* obj)
	{
	--depth;
	return TC_CONTINUE;
	}

TraversalCode ASTDumper::PreFunction(const ::Func* func)
	{
	return DoPre("Func", func);
	}

TraversalCode ASTDumper::PreStmt(const ::Stmt* stmt)
	{
	return DoPre("Stmt", stmt);
	}

TraversalCode ASTDumper::PreExpr(const ::Expr* expr)
	{
	return DoPre("Expr", expr);
	}

TraversalCode ASTDumper::PreID(const ::ID* id)
	{
	return DoPre("ID", id);
	}

TraversalCode ASTDumper::PreTypedef(const ::ID* id)
	{
	return DoPre("ID", id);
	}

TraversalCode ASTDumper::PreDecl(const ::ID* id)
	{
	return DoPre("ID", id);
	}

TraversalCode ASTDumper::PostFunction(const ::Func* func)
	{
	return DoPost("Func", func);
	}

TraversalCode ASTDumper::PostStmt(const ::Stmt* stmt)
	{
	return DoPost("Stmt", stmt);
	}

TraversalCode ASTDumper::PostExpr(const ::Expr* expr)
	{
	return DoPost("Expr", expr);
	}

TraversalCode ASTDumper::PostID(const ::ID* id)
	{
	return DoPost("ID", id);
	}

TraversalCode ASTDumper::PostTypedef(const ::ID* id)
	{
	return DoPost("ID", id);
	}

TraversalCode ASTDumper::PostDecl(const ::ID* id)
	{
	return DoPost("ID", id);
	}
