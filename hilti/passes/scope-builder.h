
#ifndef HILTI_SCOPE_BUILDER_H
#define HILTI_SCOPE_BUILDER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Fills an AST's Scope instances from declarations it finds.
class ScopeBuilder : public Pass<>
{
public:
   /// Constructor.
   ///
   /// libdirs: List of directories to search for imported library files.
   ScopeBuilder(CompilerContext* ctx) : Pass<>("ScopeBuilder") {
       _context = ctx;
   }

   virtual ~ScopeBuilder() {}

   /// Fills an AST's scopes based o the declarations found in the ast.
   ///
   /// module: The AST to process.
   bool run(shared_ptr<Node> module) override;

protected:
   void visit(Module* m) override;
   void visit(statement::Block* b) override;
   void visit(declaration::Variable* v) override;
   void visit(declaration::Type* t) override;
   void visit(declaration::Constant* t) override;
   void visit(declaration::Function* t) override;

private:
   CompilerContext* _context;
   shared_ptr<Scope> _checkDecl(Declaration* decl);
};

/// Dumps out the contents of all scopes in an AST. This is mainly for
/// debugging.
class ScopePrinter : public Pass<>
{
public:
   ScopePrinter(std::ostream& out) : Pass<>("ScopePrinter"), _out(out) {}
   virtual ~ScopePrinter() {}

   /// Dumps out the contents of all scopes in an AST.
   ///
   /// module: The AST which's scopes to dump
   ///
   /// Returns: True if no errors were encountered.
   bool run(shared_ptr<Node> module) override {
       return processAllPreOrder(module);
   }

protected:
   void visit(statement::Block* b) override;

private:
   std::ostream& _out;
};

}

}

#endif
