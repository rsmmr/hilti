
#ifndef HILTI_PRINTER_H
#define HILTI_PRINTER_H

#include "../pass.h"

namespace hilti {
namespace passes {

/// Renders a HILTI AST back into HILTI source code.
class Printer : public Pass<>
{
public:
   /// Constructor.
   ///
   /// out: Stream to write the source code representation to.
   Printer(std::ostream& out) : Pass<>("Printer"), _out(out) {}

   /// Renders a HILTI AST back into HILTI source code.
   ///
   /// module: The AST to print.
   ///
   /// Returns: True if no errors were encountered.
   bool run(shared_ptr<Node> module) override {
       return processOne(module);
   }

   virtual void reset() override;

   /// Returns the stream to print to.
   std::ostream& out() const { return _out; }

   bool _bol = true;

protected:
   void visit(Module* m) override;
   void visit(ID* id) override;

   void visit(statement::Block* b) override;
   void visit(statement::instruction::Resolved* i) override;
   void visit(statement::instruction::Unresolved* i) override;

   void visit(declaration::Variable* v) override;

   void visit(declaration::Function* f) override;
   void visit(type::function::Parameter* p) override;

   void visit(type::Any* t) override;
   void visit(type::Void* t) override;
   void visit(type::Integer* i) override;
   void visit(type::String* s) override;
   void visit(type::Bool* b) override;
   void visit(type::Tuple* t) override;
   void visit(type::Reference* t) override;
   void visit(type::Bytes* t) override;

   void visit(expression::Constant* e) override;
   void visit(expression::ID* e) override;
   void visit(expression::Variable* v) override;
   void visit(expression::Parameter* p) override;
   void visit(expression::Function* f) override;
   void visit(expression::Module* m) override;
   void visit(expression::Type* t) override;
   void visit(expression::Block* b) override;
   void visit(expression::CodeGen* c) override;
   void visit(expression::Ctor* t) override;

   void visit(constant::Integer* i) override;
   void visit(constant::String* s) override;
   void visit(constant::Bool* b) override;
   void visit(constant::Reference* r) override;
   void visit(constant::Tuple* t) override;
   void visit(constant::Unset* u) override;

   void visit(ctor::Bytes* r) override;

   void pushIndent() { ++_indent; _bol = true; }
   void popIndent()   { --_indent; }

   template<typename T>
   void printList(const std::list<T>& elems, string separator) {
       Printer& p = *this;
       bool first = true;
       for ( auto e: elems ) {
           if ( ! first )
               p << separator;

           p << e;
           first = false;
       }
   }

private:
   friend Printer& operator<<(Printer& p, Printer& (*pf)(Printer&p));
   friend Printer& operator<<(Printer& p, std::ostream& (*pf)(std::ostream&));
   friend Printer& operator<<(Printer& p, std::ios& (*pf)(std::ios&));
   friend Printer& operator<<(Printer& p, std::ios_base& (*pf)(std::ios_base&));
   template<typename T> friend Printer& operator<<(Printer& p, T t);
   template<typename T> friend Printer& operator<<(Printer& p, shared_ptr<T> node);
   template<typename T> friend Printer& operator<<(Printer& p, node_ptr<T> node);

   void prepLine() {
       if ( _bol ) {
           for ( int i = 0; i < _indent; ++i )
               _out << "    ";
       }
       _bol = false;
   }

   string scopedID(Expression* expr, shared_ptr<ID> id);

   std::ostream& _out;
   int _indent = 0;
};

// Mimic the ostream interface.

namespace printer {
    inline Printer& endl(Printer& p) {
        p.out() << std::endl;
        p._bol = true;
        return p;
    }
}

template<typename T>
inline Printer& operator<<(Printer& p, T t) { p.prepLine(); p.out() << t; return p; }

template<typename T>
inline Printer& operator<<(Printer& p, shared_ptr<T> node) { p.prepLine(); p.call(node); return p; }

template<typename T>
inline Printer& operator<<(Printer& p, node_ptr<T> node) { p.prepLine(); p.call(node); return p; }

inline Printer& operator<<(Printer& p, Printer& (*pf)(Printer&p)) { p.prepLine(); return (*pf)(p); }
inline Printer& operator<<(Printer& p, std::ios& (*pf)(std::ios&)) { p.prepLine(); p.out() << pf; return p; }
inline Printer& operator<<(Printer& p, std::ios_base& (*pf)(std::ios_base&)) { p.prepLine(); p.out() << pf; return p; }
inline Printer& operator<<(Printer& p, std::ostream& (*pf)(std::ostream&)) { p.prepLine(); p.out() << pf; return p; }

}
}

#endif
