
#ifndef AST_LOGGER_H
#define AST_LOGGER_H

#include <iostream>
#include "exception.h"
#include "location.h"

namespace ast {

/// Exception signaling a fatal error reported to a Logger via
/// Logger::fatalError().
class FatalLoggerError : public RuntimeError
{
public:
    /// Constructor.
    ///
    /// what: A string describing the error.
    FatalLoggerError(string what) : RuntimeError(what) {}
};

/// Class providing logging and debug output infrastructure. The default is
/// for an instance to do the logging directly itself, but it can also be
/// configured all output recursively to another Logger instance.
///
/// TODO: This interface needs to be redesigned to support:
///
/// - debug streams instead of levels.
///
/// - C++-style stream output ("<<").
///
/// No documentation until that's done.
class Logger
{
public:
    Logger(const string& name = "", std::ostream& output = std::cerr) : _output(output) {
       _name = name;
    }

    Logger(const string& name, Logger* forward_to) {
       _forward = forward_to;
       _name = name;
    }

    virtual ~Logger();

    const string& loggerName() const { return _name; }
    void setLoggerName(const string& name) { _name = name; }

    void forwardLoggingTo(Logger* logger) { _forward = logger; }

    void fatalError(const string& message, const string& location = "") const;
    void fatalError(const string& message, shared_ptr<NodeBase> node) const;
    void fatalError(const string& message, NodeBase* node) const;

    void error(const string& message, const string& location = "") const;
    void error(const string& message, shared_ptr<NodeBase> node) const;
    void error(const string& message, NodeBase* node) const;

    void internalError(const string& message, const string& location = "") const;
    void internalError(const string& message, shared_ptr<NodeBase> node) const;
    void internalError(const string& message, NodeBase* node) const;

    void warning(const string& message, const string& location = "") const;
    void warning(const string& message, shared_ptr<NodeBase> node) const;
    void warning(const string& message, NodeBase* node) const;

    int errors() const { return _forward ? _forward->errors() : _errors; }
    int warnings() const { return _forward ? _forward->warnings() : _errors; }
    void reset() { _errors = _warnings = 0; }

    int debugLevel() const;
    void debugSetLevel(int level);
    void debug(int level, string msg);
    void debugPushIndent();
    void debugPopIndent();

private:
    string _name;
    std::ostream& _output = std::cerr;
    Logger* _forward = nullptr;
    mutable int _warnings = 0;
    mutable int _errors = 0;

    int _debug_level = 0;
    int _debug_indent = 0;

protected:
    enum ErrorType { Warning, Error, Internal, Fatal };
    virtual void doError(const string& message, NodeBase* node, const string& location, ErrorType type) const;
};

}

#endif
