
#ifndef AST_EXCEPTION_H
#define AST_EXCEPTION_H

#include "node.h"

namespace ast {

/// Base class for all AST exceptions.
class Exception : public std::exception {
public:
    /// Constructor. Optionally, a Node can be associated with the exception.
    ///
    /// what: A textual description. Will be passed to std::exception.
    ///
    /// node: The optional Node.
    Exception(string what, const NodeBase* node = 0) throw()
    {
        _what = what;
        _node = node;
    }

    virtual ~Exception() throw()
    {
    }

    /// Returns the Node associated with the exception.
    const NodeBase* node() const
    {
        return _node;
    }

    /// Returns the associated Node's location. The return value will be
    /// Location::None if no node has been set.
    const Location& location() const
    {
        return _node ? _node->location() : Location::None;
    }

    // Returns the textual description. If the exception has a Node associated
    // that has a location, that is added automatically.
    virtual const char* what() const throw();

private:
    // Exception(const Exception&) = delete;  // FIXME: Not sure why I can't delete this here.
    Exception& operator=(const Exception&) = delete;

    string _what;
    const NodeBase* _node;
};

/// Exception reporting an run-time error due to unexpected user input.
class RuntimeError : public Exception {
public:
    /// Constructor. Optionally, a Node can be associated with the exception.
    ///
    /// what: A textual description. Will be passed to std::exception.
    ///
    /// node: The optional Node.
    RuntimeError(string what, const NodeBase* node = 0) : Exception(what, node)
    {
    }
};

/// Exception reporting an internal logic error.
class InternalError : public Exception {
public:
    /// Constructor. Optionally, a Node can be associated with the exception.
    ///
    /// what: A textual description. Will be passed to std::exception.
    ///
    /// node: The optional Node.
    InternalError(string what, const NodeBase* node = 0) : Exception(what, node)
    {
    }
};
}

#endif
