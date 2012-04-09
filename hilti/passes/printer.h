
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
   ///
   /// single_line: If true, all line separator while be turned into space so
   /// that we get a single-line version of the output.
   Printer(std::ostream& out, bool single_line=false) : Pass<>("Printer"), _out(out), _single_line(single_line) {}

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

   /// Returns true if single-line output is desired.
   bool singleLine() const { return _single_line; }

   bool _bol = true;
   bool _no_indent = false;

protected:
   void visit(Module* m) override;
   void visit(ID* id) override;

   void visit(statement::Block* s) override;
   void visit(statement::Try* s) override;
   void visit(statement::instruction::Resolved* s) override;
   void visit(statement::instruction::Unresolved* s) override;
   void visit(statement::try_::Catch* s) override;

   void visit(declaration::Variable* v) override;
   void visit(declaration::Function* f) override;
   void visit(declaration::Type* t) override;

   void visit(type::function::Parameter* p) override;

   void visit(type::Address* t) override;
   void visit(type::Any* t) override;
   void visit(type::Bitset* t) override;
   void visit(type::Bool* t) override;
   void visit(type::Bytes* t) override;
   void visit(type::CAddr* t) override;
   void visit(type::Callable* t) override;
   void visit(type::Channel* t) override;
   void visit(type::Classifier* t) override;
   void visit(type::Double* t) override;
   void visit(type::Enum* t) override;
   void visit(type::Exception* t) override;
   void visit(type::File* t) override;
   void visit(type::Function* t) override;
   void visit(type::Hook* t) override;
   void visit(type::IOSource* t) override;
   void visit(type::Integer* t) override;
   void visit(type::Interval* t) override;
   void visit(type::Iterator* t) override;
   void visit(type::List* t) override;
   void visit(type::Map* t) override;
   void visit(type::MatchTokenState* t) override;
   void visit(type::Network* t) override;
   void visit(type::Overlay* t) override;
   void visit(type::OptionalArgument* t) override;
   void visit(type::Port* t) override;
   void visit(type::Reference* t) override;
   void visit(type::RegExp* t) override;
   void visit(type::Set* t) override;
   void visit(type::String* t) override;
   void visit(type::Struct* t) override;
   void visit(type::Time* t) override;
   void visit(type::Timer* t) override;
   void visit(type::TimerMgr* t) override;
   void visit(type::Tuple* t) override;
   void visit(type::Type* t) override;
   void visit(type::Unknown* t) override;
   void visit(type::Vector* t) override;
   void visit(type::Void* t) override;

   void visit(expression::Block* e) override;
   void visit(expression::CodeGen* e) override;
   void visit(expression::Coerced* e) override;
   void visit(expression::Constant* e) override;
   void visit(expression::Ctor* e) override;
   void visit(expression::Function* e) override;
   void visit(expression::ID* e) override;
   void visit(expression::Module* e) override;
   void visit(expression::Parameter* e) override;
   void visit(expression::Type* e) override;
   void visit(expression::Variable* e) override;

   void visit(constant::Address* c) override;
   void visit(constant::Bitset* c) override;
   void visit(constant::Bool* c) override;
   void visit(constant::Double* c) override;
   void visit(constant::Enum* c) override;
   void visit(constant::Integer* c) override;
   void visit(constant::Interval* c) override;
   void visit(constant::Label* c) override;
   void visit(constant::Network* c) override;
   void visit(constant::Port* c) override;
   void visit(constant::Reference* c) override;
   void visit(constant::String* c) override;
   void visit(constant::Time* c) override;
   void visit(constant::Tuple* c) override;
   void visit(constant::Unset* c) override;

   void visit(ctor::Bytes* c) override;
   void visit(ctor::List* c) override;
   void visit(ctor::Map* c) override;
   void visit(ctor::RegExp* c) override;
   void visit(ctor::Set* c) override;
   void visit(ctor::Vector* c) override;

   void pushIndent() { ++_indent; _bol = true; }
   void popIndent()   { --_indent; }

   template<typename T>
   void printList(const std::list<T>& elems, const string& separator, const string& elem_prefix = "", const string& elem_postfix = "") {
       Printer& p = *this;
       bool first = true;
       for ( auto e: elems ) {
           if ( ! first )
               p << separator;

           p << elem_prefix << e << elem_postfix;
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
       if ( _bol && ! _no_indent ) {
           for ( int i = 0; i < _indent; ++i )
               _out << "    ";
       }
       _bol = false;
       _no_indent = false;
   }

   string scopedID(Expression* expr, shared_ptr<ID> id);

   // Prints a type's ID if we have it and doing so was not disabled via,
   // disableTypeIDs(). If that's both the case returns true, else false
   bool printTypeID(shared_ptr<Type> t) { return printTypeID(t.get()); }
   bool printTypeID(Type* t);

   // Disables printing IDs for declared types. Calls to this method must match
   // those of enableTypeIDs().
   void disableTypeIDs() { --_print_type_ids; }

   // Enables printing IDs for declared types. Calls to this method must
   // match those of enableTypeIDs().
   void enableTypeIDs() { ++_print_type_ids; }

   std::ostream& _out;
   bool _single_line;
   int _indent = 0;
   int _print_type_ids = 1;
};

// Mimic the ostream interface.

namespace printer {
    inline Printer& endl(Printer& p) {
        if ( p.singleLine() )
            p.out() << ' ';
        else
            p.out() << std::endl;

        p._bol = true;
        return p;
    }

    inline Printer& no_indent(Printer& p) {
        p._no_indent = true;
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
