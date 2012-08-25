
#ifndef AST_FUNCTION_H
#define AST_FUNCTION_H

#include <list>

#include <util/util.h>

#include "node.h"

namespace ast {

/// An AST node describing a function.
template<typename AstInfo>
class Function : public AstInfo::node
{
public:
    typedef typename AstInfo::node Node;
    typedef typename AstInfo::type Type;
    typedef typename AstInfo::statement Statement;
    typedef typename AstInfo::module Module;
    typedef typename AstInfo::function_type FunctionType;
    typedef typename AstInfo::id ID;

    /// Constructor.
    ///
    /// id: A non-scoped ID with the function's name.
    ///
    /// module: The module the function is part of. Note that it will not
    /// automatically be added to that module.
    ///
    /// body: A statement with the function's body. Typically, the statement type will be that of a block of statements.
    ///
    /// l: Location associated with the node.
    Function(shared_ptr<ID> id, shared_ptr<FunctionType> ftype, shared_ptr<Module> module, shared_ptr<Statement> body = nullptr, const Location& l=Location::None) : Node(l) {
       _id = id;
       this->addChild(_id);

       _ftype = ftype;
       this->addChild(_ftype);

       _module = module;

       // Don't add module as child here, that would create a cycle.

       if ( body )
           setBody(body);
    }

    /// Returns the types of the function.
    shared_ptr<FunctionType> type() const {
       return _ftype;
    }

    /// Returns the module the function is part of.
    shared_ptr<Module> module() const {
       return _module;
    }

    /// Returns the body of the function.
    shared_ptr<Statement> body() const { return _body; }

    /// Sets the body of the function.
    ///
    /// body: The statement that makes up the body (typically something like a block-statement).
    void setBody(shared_ptr<Statement> body) {
       _body = body;
       this->addChild(_body);
    }

    /// Returns the ID with the function's name. It's non-scoped.
    shared_ptr<ID> id() const { return _id; }

    string render() /* override */ {
        return util::fmt("%s (%s)", _id ? _id->name().c_str() : "<no id>", _ftype ? _ftype->render().c_str() : "<no type>");
    }

private:
    node_ptr<Statement> _body;
    node_ptr<FunctionType> _ftype;
    node_ptr<Module> _module;
    node_ptr<ID> _id;
};

}

#endif
