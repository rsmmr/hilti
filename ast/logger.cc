
#include "logger.h"

using namespace ast;

Logger::~Logger()
{
}

void Logger::error(const string& message, const string& location)
{
    if ( _forward ) {
        _forward->error(message, location);
        return;
    }

    ++_errors;
    doError(message, location, Error);
}

void Logger::internalError(const string& message, const string& location)
{
    if ( _forward ) {
        _forward->internalError(message, location);
        return;
    }

    ++_errors;
    doError(message, location, Internal);
    abort();
}

void Logger::fatalError(const string& message, const string& location)
{
    if ( _forward ) {
        _forward->fatalError(message, location);
        return;
    }

    ++_errors;
    doError(message, location, Fatal);
    throw FatalLoggerError(message);
}

void Logger::warning(const string& message, const string& location)
{
    if ( _forward ) {
        _forward->warning(message, location);
        return;
    }

    ++_warnings;
    doError(message, location, Warning);
}

void Logger::doError(const string& message, const string& location, ErrorType type)
{
    string tag;

    switch ( type ) {
     case Warning:
        tag = "warning";
        break;

     case Error:
        tag = "error";
        break;

     case Internal:
        tag = "internal error";
        break;

     case Fatal:
        tag = "fatal error";
        break;

     default:
        assert(false);
    }


    if ( location.size() )
        _output << location << ": ";

    if ( _name.size() )
        _output << _name << " ";

    _output << tag << ", " << message << std::endl;

#if 0
    if ( type == Fatal || type == Internal )
        _output << "aborting." << std::endl;
#endif        
}

#ifdef DEBUG

int Logger::debugLevel() const
{
    if ( _forward )
        return _forward->debugLevel();

    return _debug_level;
}

void Logger::debugSetLevel(int level)
{
    if ( _forward ) {
        _forward->debugSetLevel(level);
        return;
    }

    _debug_level = level;
}

void Logger::debugPushIndent()
{
    if ( _forward ) {
        _forward->debugPushIndent();
        return;
    }

    ++_debug_indent;
}

void Logger::debugPopIndent()
{
    if ( _forward ) {
        _forward->debugPopIndent();
        return;
    }

    --_debug_indent;
}

void Logger::debug(int level, string msg) {
    if ( _debug_level < level )
           return;

    if ( _name.size() )
        std::cerr << "debug: [" << _name << "] ";
    else
        std::cerr << "debug: [" << typeid(*this).name() << "] ";

    for ( int i = 0; i < _debug_indent; ++i )
        std::cerr << "    ";

    std::cerr << msg << std::endl;
}
#endif

