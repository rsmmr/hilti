
#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "../pass.h"

namespace ast {
namespace passes {

/// Base class for converting an AST back into source code.
template<typename AstInfo>
class Printer : public Pass<AstInfo>
{
public:
    /// Constructor.
    ///
    /// out: Stream to write the source code representation to.
    ///
    /// single_line: If true, all line separators while be turned into space
    /// so that we get a single-line version of the output.
    Printer(std::ostream& out, bool single_line = false)
       : Pass<AstInfo>("Printer"), _out(out), _single_line(single_line) {}

    /// Renders an AST back into HILTI source code.
    ///
    /// module: The AST to print.
    ///
    /// Returns: True if no errors were encountered.
    bool run(shared_ptr<NodeBase> module) /* override */ {
       return Visitor<AstInfo>::processOne(module);
    }

    /// Returns the stream to print to.
    std::ostream& out() const { return _out; }

    /// Returns true if single-line output is desired.
    bool singleLine() const { return _single_line; }

    virtual void reset() override {
       Visitor<AstInfo>::reset();
       _indent = 0;
       _bol = true;
       _print_type_ids = 1;
       _linefeeds = 0;
    }

    // These should really be private.
    bool _bol = true;
    bool _no_indent = false;
    int  _linefeeds = 0;
    int  _print_type_ids = 1;

    /// Increases indentation level in output by one.
    void pushIndent() { ++_indent; if ( ! _bol ) { *this << "\n"; _bol = true; } }

    /// Decreases indentation level in output by one.
    void popIndent()  { --_indent; if ( ! _bol ) { *this << "\n"; _bol = true; } }

    /// A helper method that printers a list of items, separated by a
    /// separator.
    ///
    /// elems: The elements to printe. They'll be rendered via their \c << operator.
    ///
    /// separator: The separator to insert between elements.
    ///
    /// elem_prefix: A prefix to insert before each element.
    ///
    /// elem_postfix: A postfix to insert after each element.
    ///
    /// separator
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

    template<typename T>
    void printList(const std::list<T>& elems, const string& separator, const string& elem_prefix, const string& elem_postfix,
                   std::function<string (T&)> fmt) {
        Printer& p = *this;
        bool first = true;
        for ( auto e: elems ) {
            if ( ! first )
                p << separator;

            p << elem_prefix << fmt(e) << elem_postfix;
            first = false;
        }
    }

    /// Disables printing IDs for declared types. Calls to this method must
    /// match those of enableTypeIDs().
    void disableTypeIDs() { --_print_type_ids; }

    /// Enables printing IDs for declared types. Calls to this method must
    /// match those of enableTypeIDs().
    void enableTypeIDs() { ++_print_type_ids; }

    /// XXX
    void setPrintOriginalIDs(bool org) { _orginal_ids = org; }

    /// XXX
    bool printOriginalIDs() const { return _orginal_ids; }

    void printMetaInfo(const MetaInfo* meta) {
        if ( ! meta->size() )
            return;

        Printer& p = *this;
        p << " # " << string(*meta);
    }

protected:
    void printDebug(shared_ptr<NodeBase> node) override {
        // No debug printing as they would recurse.
    }

private:
    template<typename AI> friend Printer<AI>& operator<<(Printer<AI>& p, Printer<AI>& (*pf)(Printer<AI>&p));
    template<typename AI> friend Printer<AI>& operator<<(Printer<AI>& p, std::ostream& (*pf)(std::ostream&));
    template<typename AI> friend Printer<AI>& operator<<(Printer<AI>& p, std::ios& (*pf)(std::ios&));
    template<typename AI> friend Printer<AI>& operator<<(Printer<AI>& p, std::ios_base& (*pf)(std::ios_base&));
    template<typename AI, typename T> friend Printer<AI>& operator<<(Printer<AI>& p, T t);
    template<typename AI, typename T> friend Printer<AI>& operator<<(Printer<AI>& p, shared_ptr<T> node);
    template<typename AI, typename T> friend Printer<AI>& operator<<(Printer<AI>& p, node_ptr<T> node);

    void _prepLine() {
       if ( _bol && ! _no_indent ) {
           for ( int i = 0; i < _indent; ++i )
               _out << "  ";
       }
       _bol = false;
       _no_indent = false;
       _linefeeds = 0;
    }

    std::ostream& _out;
    bool _single_line;
    int _indent = 0;
    bool _orginal_ids = false;
};

namespace printer {

template<typename AstInfo, typename Expr, typename Id>
string scopedID(Printer<AstInfo>* printer, Expr expr, Id id)
{
    if ( printer->printOriginalIDs() && id->original() )
        id = id->original();

    if ( expr && expr->scope().size() ) {
        auto path = id->pathAsString();
        auto scope = expr->scope() + "::";
        return ::util::startsWith(path, scope) ? path : scope + path;
    }
    else
        return id->pathAsString();
}

template<typename AstInfo, typename Ty>
bool printTypeID(Printer<AstInfo>* printer, Ty* t)
{
    if ( ! printer->_print_type_ids )
        return false;

    if ( ! t->id() )
        return false;

    (*printer) << t->id()->pathAsString();
    return true;
}

// Mimic the ostream interface.
template<typename AstInfo>
inline Printer<AstInfo>& endl(Printer<AstInfo>& p)
{
    if ( p.singleLine() )
        p.out() << ' ';
    else if ( ! p._linefeeds )
        p.out() << std::endl;

    ++p._linefeeds;
    p._bol = true;
    return p;
}

template<typename AstInfo>
inline Printer<AstInfo>& no_indent(Printer<AstInfo>& p) {
    p._no_indent = true;
    return p;
}

}

template<typename AstInfo, typename T>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, T t) {
    p._prepLine(); p.out() << t; return p;
}

template<typename AstInfo, typename T>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, shared_ptr<T> node)
{
    p._prepLine();

    if ( node )
        p.call(node);
    else
        p << "<nullptr>";

    return p;
}

template<typename AstInfo, typename T>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, node_ptr<T> node)
{
    p._prepLine();

    if ( node )
        p.call(node);
    else
        p << "<nullptr>";

    return p;
}

template<typename AstInfo>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, Printer<AstInfo>& (*pf)(Printer<AstInfo>&p)) {
    p._prepLine();
    return (*pf)(p);
}

template<typename AstInfo>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, std::ios& (*pf)(std::ios&)) {
    p._prepLine();
    p.out() << pf;
    return p;
}

template<typename AstInfo>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, std::ios_base& (*pf)(std::ios_base&)) {
    p._prepLine();
    p.out() << pf;
    return p;
}

template<typename AstInfo>
inline Printer<AstInfo>& operator<<(Printer<AstInfo>& p, std::ostream& (*pf)(std::ostream&)) {
    p._prepLine();
    p.out() << pf;
    return p;
}

}

}

#endif
