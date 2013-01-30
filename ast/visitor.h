
#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include <list>
#include <set>

#include <util/util.h>

#include "exception.h"
#include "node.h"
#include "logger.h"

namespace ast {

// TODO: Document the macros.

// FIXME: The methods should all be declared "override" as well but that
// gives compiler errors.

#define ACCEPT_VISITOR(base)                          \
    void accept(VisitorInterface* visitor) override { \
       this->base::accept(visitor);                   \
       visitor->visit(this);                          \
    }                                                 \
    const char* acceptClass() const override {        \
       return typeid(*this).name();                   \
    }

#define ACCEPT_VISITOR_ROOT()                         \
    void accept(VisitorInterface* visitor) /* override */ { \
       visitor->visit(this);                          \
    }                                                 \
    const char* acceptClass() const  {        \
       return typeid(*this).name();                  \
    }

#define ACCEPT_VISITOR_FORWARD_TO_BASE(base)         \
    void accept(VisitorInterface* visitor) /* override */ { \
       this->base::accept(visitor);                  \
    }                                                 \
    const char* acceptClass() const  {        \
       return typeid(*this).name();                  \
    }

#define ACCEPT_DISABLED                              \
    void accept(VisitorInterface* visitor) /* override */ { \
       throw InternalError("disabled accept called", this); \
    }                                                 \
    const char* acceptClass() const  {        \
       return typeid(*this).name();                  \
    }

/// Enables verbose debugging for *all* visitors.
extern void enableDebuggingForAllVisitors(bool enabled = true);

/// Returns true if verbose debugging is enabled for all visitor.
extern bool debuggingAllVisitors();

/// Base class for all AST visitors.
///
/// The class supports different types of visitors, including:
///
///    - Visitors that iterate over all nodes in pre-/post order.
///
///    - Visitors that want to control the iteration themselves by having their
///      visit methods forward to child nodes as desired.
///
///    - Visitors with visit methods that receive up to 2 further arguments,
///    in addition to the Node currently being visited.
///
///    - Visitors that compute and return value.
///
/// All these can also be mixed. Argument and result types are parameterized
/// by template parameters.
///
/// The visit methods themselves are outsource to a client-specific
/// VisitorInterface. That is also passed in as a template parameter,
template<typename AstInfo, typename Result=int, typename Arg1=int, typename Arg2=int>
class Visitor : public AstInfo::visitor_interface, public Logger
{
public:
    typedef typename AstInfo::visitor_interface VisitorInterface;
    typedef std::list<shared_ptr<NodeBase>> node_list;

    /// Processes all child nodes pre-order, i.e., parent nodes are visited
    /// before their childs.
    ///
    /// node: The node where to start visiting.
    ///
    /// reverse: If true, visits siblings in reverse order.
    bool processAllPreOrder(shared_ptr<NodeBase> node, bool reverse = false);

    /// Processes all child nodes pre-order, i.e., parent nodes are visited
    /// after their childs.
    ///
    /// node: The node where to start visiting.
    ///
    /// reverse: If true, visits siblings in reverse order.
    bool processAllPostOrder(shared_ptr<NodeBase> node, bool reverse = false);

    /// Visits just a single node. The methods doesn't recurse any further,
    /// although the individual visit methods can manually do so by invoking
    /// any of the #call methods.
    ///
    /// In other words, this method is external interface to visiting a single
    /// node, whereas *call* is internal interface when a visiting process is
    /// already in progress. Don't mix the two.
    ///
    /// node: The node to visit.
    ///
    /// Returns: True if no fatalError() has been reported.
    bool processOne(shared_ptr<NodeBase> node) {
        saveArgs();
        auto b = processOneInternal(node);
        restoreArgs();
        return b;
    }

    /// Like processOne(), but also allows the visit method to set a result
    /// value. If no result is set via setResult(), the default set with
    /// setDefault() is returned. If neither is set, that's an error.
    ///
    /// node: The node to visit.
    ///
    /// result: Pointer to instance to store result in.
    bool processOne(shared_ptr<NodeBase> node, Result* result) {
        saveArgs();
        auto b = processOneInternal(node, result);
        restoreArgs();
        return b;
    }

    /// Like processOne(), but also sets the visitors first argument. Visit
    /// methods can then retrieve it with #arg1.
    bool processOne(shared_ptr<NodeBase> node, Arg1 arg1) {
        saveArgs();
        this->setArg1(arg1);
        auto r = processOneInternal(node);
        restoreArgs();
        return r;
    }

    /// Like processOne(), but also sets the visitors first and second
    /// arguments. Visit methods can then retrieve it with #arg1 and #arg2.
    bool processOne(shared_ptr<NodeBase> node, Arg1 arg1, Arg2 arg2) {
        saveArgs();
        this->setArg1(arg1);
        this->setArg2(arg2);
        auto r = processOneInternal(node);
        restoreArgs();
        return r;
    }

    /// Like processOne(), with setting an argument and getting a result back.
    bool processOne(shared_ptr<NodeBase> node, Result* result, Arg1 arg1) {
        saveArgs();
        this->setArg1(arg1);
        auto r = processOneInternal(node, result);
        restoreArgs();
        return r;
    }

    /// Like processOne(), with setting arguments and getting a result back.
    bool processOne(shared_ptr<NodeBase> node, Result* result, Arg1 arg1, Arg2 arg2) {
        saveArgs();
        this->setArg1(arg1);
        this->setArg2(arg2);
        auto r = processOneInternal(node, result);
        restoreArgs();
        return r;
    }

    /// Returns true if we currently in a visit method that was triggered by
    /// recursing down from a higher-level method for type T. The result is
    /// undefined if there's currently no visiting in progress.
    template<typename T>
    bool in() {
       return current<T>(typeid(T)) != 0;
    }

    /// Returns the closest higher-level instance of a given type. This method
    /// must only be called during an ongoing visiting process from another
    /// visit method. It will return a higher-level objects of type T, or null
    /// if there's no such. Note that this may return the current instance,
    /// use parent() if that's not desired.
    template<typename T>
    shared_ptr<T> current() {
       auto t = current<T>(typeid(T));
       return t;
    }

    Location currentLocation() {
       for ( auto i = _current.rbegin(); i != _current.rend(); i++ ) {
           if ( (*i)->location() != Location::None )
               return (*i)->location();
       }

       return Location::None;
    }

    /// Returns the closest higher-level instance of a given type (excluding
    /// the current one). This method must only be called during an ongoing
    /// visiting process from another visit method. It will return a
    /// higher-level objects of type T, or null if there's no such.
    template<typename T>
    shared_ptr<T> parent() {
       auto t = current<T, true>(typeid(T));
       return t;
    }

    /// Returns the number of higher-level instance of a given type. This method
    /// must only be called during an ongoing visiting process from another
    /// visit method.
    template<typename T>
    shared_ptr<T> depth() {
       int d = 0;
       for ( auto i = _current.rbegin(); i != _current.rend(); i++ ) {
           if ( typeid(**i) == typeid(T) )
               ++d;
       }

       return d;
    }

    /// Returns the current list of nodes we have traversed so far from the
    /// top of the tree to get to the current node. The latter is included.
    const node_list& currentNodes() const { return _current; }

    /// Dumps the list of currentNodes(). This is primarily for
    /// debugging to track down the chain of nodes.
    void dumpCurrentNodes(std::ostream& out) {
        out << "Current visitor nodes:" << std::endl;
        for ( auto n : currentNodes() )
            out << "   | " << *n << std::endl;
        out << std::endl;
    }

    /// Returns the first argument given to the visitor. The argument can be
    /// set by calling setArg1() prior to starting the visiting via any of \c
    /// process* methods. The return value is undefined if no argument has
    /// been set.
    Arg1 arg1() const { return _arg1; }

    /// Returns the second argument given to the visitor. The argument can be
    /// set by calling setArg2() prior to starting the visiting via any of \c
    /// process* methods. The return value is undefined if no argument has
    /// been set.
    Arg2 arg2() const { return _arg2; }

    /// Sets the result to be returned by any of the \c *withResult() methods.
    /// This must only be called from a visit method during traversal.
    void setResult(Result result) {
       _result_set = true;
       _result = result;
    }

    /// Recurse to another node. Must only be called by a visit method during
    /// traversal.
    void call(shared_ptr<NodeBase> node) {
       this->pushCurrent(node);
       this->printDebug(node);
       this->preAccept(node);
       this->callAccept(node);
       this->postAccept(node);
       this->popCurrent();
    }

    /// Returns the current recursion level during visiting.
    int level() const { return _level; }

    /// Returns a string of whitespace reflecting an indentation according to
    /// the current recursion level.
    string levelIndent() const;

    /// XXX
    void pushCurrentLocationNode(shared_ptr<NodeBase> node) { _loc_nodes.push_back(node.get()); }

    /// XXXX
    void pushCurrentLocationNode(NodeBase* node) { _loc_nodes.push_back(node); }

    /// XXX
    void popCurrentLocationNode() { _loc_nodes.pop_back(); }

    const NodeBase* currentLocationNode() const { return _loc_nodes.back(); }

    /// Forwards to Logger.
    void error(NodeBase* node, string msg) const {
       Logger::error(msg, _loc_nodes.back() ? _loc_nodes.back() : node);
    }

    /// Forwards to Logger.
    void error(shared_ptr<NodeBase> node, string msg) const {
       Logger::error(msg, _loc_nodes.back() ? _loc_nodes.back() : node.get());
    }

    /// Forwards to Logger.
    void error(string msg) {
       Logger::error(msg, _loc_nodes.back());
    }

    /// Forwards to Logger.
    void internalError(NodeBase* node, string msg) const {
       Logger::internalError(msg, _loc_nodes.back() ? _loc_nodes.back() : node);
    }

    /// Forwards to Logger.
    void internalError(shared_ptr<NodeBase> node, string msg) const {
       Logger::internalError(msg, _loc_nodes.back() ? _loc_nodes.back() : node.get());
    }

    /// Forwards to Logger.
    void internalError(string msg) const {
       Logger::internalError(msg, _loc_nodes.back());
    }

    /// Forwards to Logger.
    void fatalError(NodeBase* node, string msg) const {
       Logger::fatalError(msg, _loc_nodes.back() ? _loc_nodes.back() : node);
    }

    /// Forwards to Logger.
    void fatalError(shared_ptr<NodeBase> node, string msg) const {
       Logger::fatalError(msg, _loc_nodes.back() ? _loc_nodes.back() : node.get());
    }

    /// Forwards to Logger.
    void fatalError(string msg) const {
       Logger::fatalError(msg, _loc_nodes.back());
    }

    /// Forwards to Logger.
    void warning(NodeBase* node, string msg) const {
       Logger::warning(msg, _loc_nodes.back() ? _loc_nodes.back() : node);
    }

    /// Forwards to Logger.
    void warning(shared_ptr<NodeBase> node, string msg) const {
       Logger::warning(msg, _loc_nodes.back() ? _loc_nodes.back() : node.get());
    }

    /// Forwards to Logger.
    void warning(string msg) const {
       Logger::warning(msg, _loc_nodes.back());
    }

    /// Enables verbose debugging for this visitor. This will log all
    /// visits()to stderr.
    void enableDebugging(bool enabled) {
        _debug = enabled;
    }

    /// Returns whether visit() logging is enabled (either specifically for
    /// this visitor, or globally for all).
    bool debugging() const {
        return _debug || debuggingAllVisitors();
    }

protected:
    /// Sets the first argument. visit methods can then retrieve it with
    /// #arg1.
    ///
    /// This method is protected because it should normally be called from a
    /// wrapper method provided by the derived visitor class as its main entry
    /// point.
    void setArg1(Arg1 arg1) { _arg1 = arg1; }

    /// Sets the first argument. visit methods can then retrieve it with
    /// #arg2.
    ///
    /// This method is protected because it should normally be called from a
    /// wrapper method provided by the derived visitor class as its main entry
    /// point.
    void setArg2(Arg2 arg2) { _arg2 = arg2; }

    /// Sets the default result to be returned by any of the \c *withResult()
    /// methods if no visit methods sets one. It's an error if no node sets a
    /// result without a default being defined.
    ///
    /// This method is protected because it should normally be called from a
    /// wrapper method provided by the derived visitor class as its main entry
    /// point.
    void setDefaultResult(Result def)  {
       _default = def;
       _default_set = true;
    }

    /// Resets the visiting process, clearing all previous state. Derived
    /// classes can override this but must call the parent implementations.
    virtual void reset() {
       _visited.clear();
       Logger::reset();
    }

    /// Called just before a node's accept(). The default implementation does nothing.
    virtual void preAccept(shared_ptr<ast::NodeBase> node) {};
    virtual void postAccept(shared_ptr<ast::NodeBase> node) {};

    void preOrder(shared_ptr<NodeBase> node, bool reverse = false);
    void postOrder(shared_ptr<NodeBase> node, bool reverse = false);

    // Prints the debug output for the current visit() call.
    virtual void printDebug(shared_ptr<NodeBase> node) {
#ifdef DEBUG
        if ( ! debugging() )
             return;

        std::cerr << '[' << this->loggerName() << "::visit()]";

        for ( auto i = _current.size(); i > 0; --i )
            std::cerr << "  ";

        std::cerr << string(*node) << std::endl;
#endif
    }

private:

    void pushCurrent(shared_ptr<NodeBase> node) {
       _current.push_back(node);
       debugPushIndent();
       ++_level;
    }

    void popCurrent() {
       _current.pop_back();
       debugPopIndent();
       --_level;
    }

    template<typename T, bool skip_first=false>
    shared_ptr<T> current(const std::type_info& ti) const {
       auto i = _current.rbegin();

       if ( skip_first )
           ++i;

       for ( ; i != _current.rend(); ++i ) {
           if ( typeid(**i) == ti )
               return std::dynamic_pointer_cast<T>(*i);
       }

       return shared_ptr<T>();
    }

    bool processOneInternal(shared_ptr<NodeBase> node);
    bool processOneInternal(shared_ptr<NodeBase> node, Result* result);
    void saveArgs();
    void restoreArgs();

    node_list _current;

    Arg1 _arg1;
    Arg2 _arg2;
    int _errors = 0;
    int _level = 0;
    bool _debug = false;

    std::list<NodeBase *> _loc_nodes = { nullptr }; // Initial null element.

    bool _result_set = false;
    Result _result;

    bool _default_set = false;
    Result _default;

    typedef std::set<shared_ptr<NodeBase>> node_set;
    node_set _visited;

    struct SavedArgs {
        Arg1 arg1;
        Arg2 arg2;
        bool result_set;
        bool default_set;
        Result result;
        Result default_;
    };

    std::list<SavedArgs> _saved_args;
};

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline bool Visitor<AstInfo, Result, Arg1, Arg2>::processAllPreOrder(shared_ptr<NodeBase> node, bool reverse)
{
    reset();

    try {
        preOrder(node, reverse);
        return (errors() == 0);
    }

    catch ( const FatalLoggerError& err ) {
        // Message has already been printed.
        return false;
    }
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline bool Visitor<AstInfo, Result, Arg1, Arg2>::processAllPostOrder(shared_ptr<NodeBase> node, bool reverse)
{
    reset();

    try {
        postOrder(node, reverse);
        return (errors() == 0);
    }

    catch ( const FatalLoggerError& err ) {
        // Message has already been printed.
           return false;
    }
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline void Visitor<AstInfo, Result, Arg1, Arg2>::saveArgs()
{
    SavedArgs args;
    args.arg1 = _arg1;
    args.arg2 = _arg2;
    args.result_set = _result_set;
    args.default_set = _default_set;
    args.result = _result;
    args.default_ = _default;
    _saved_args.push_back(args);
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline void Visitor<AstInfo, Result, Arg1, Arg2>::restoreArgs()
{
    auto args = _saved_args.back();
    _arg1 = args.arg1;
    _arg2 = args.arg2;
    _result_set = args.result_set;
    _default_set = args.default_set;
    _result = args.result;
    _default = args.default_;
    _saved_args.pop_back();
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline bool Visitor<AstInfo, Result, Arg1, Arg2>::processOneInternal(shared_ptr<NodeBase> node)
{
    reset();

    try {
        this->call(node);
        return (errors() == 0);
    }

    catch ( const FatalLoggerError& err ) {
           // Message has already been printed.
        return false;
    }
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline bool Visitor<AstInfo, Result, Arg1, Arg2>::processOneInternal(shared_ptr<NodeBase> node, Result* result)
{
    assert(result);

    reset();

    try {
        _result_set = false;
        this->call(node);

        if ( _result_set )
            *result = _result;
        else if ( _default_set )
            *result = _default;
        else
            internalError(node, util::fmt("visit method in %s did not set result for %s", typeid(*this).name(), node->acceptClass()));

        return true;
    }

    catch ( const FatalLoggerError& err ) {
        // Message has already been printed.
        return false;
    }
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline void Visitor<AstInfo, Result, Arg1, Arg2>::preOrder(shared_ptr<NodeBase> node, bool reverse)
{
    if ( _visited.find(node) != _visited.end() )
        // Done already.
        return;

    _visited.insert(node);

    pushCurrent(node);
    this->printDebug(node);
    this->callAccept(node);

    if ( ! reverse ) {
        for ( auto i = node->begin(); i != node->end(); i++ )
            this->preOrder(*i);
    }

    else {
        for ( auto i = node->rbegin(); i != node->rend(); i++ )
            this->preOrder(*i);
    }

    popCurrent();
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline void Visitor<AstInfo, Result, Arg1, Arg2>::postOrder(shared_ptr<NodeBase> node, bool reverse)
{
    if ( _visited.find(node) != _visited.end() )
        // Done already.
        return;

    _visited.insert(node);

    pushCurrent(node);

    if ( ! reverse ) {
        for ( auto i = node->begin(); i != node->end(); i++ )
            this->postOrder(*i);
    }

    else {
        for ( auto i = node->rbegin(); i != node->rend(); i++ )
            this->postOrder(*i);
    }

    this->printDebug(node);
    this->callAccept(node);
    popCurrent();
}

template<typename AstInfo, typename Result, typename Arg1, typename Arg2>
inline string Visitor<AstInfo, Result, Arg1, Arg2>::levelIndent() const {
       auto s = string("");
       for ( int i = 0; i < _level; ++i )
           s += "  ";
       return s;
    }


}

#endif
