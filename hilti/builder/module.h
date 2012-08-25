
#ifndef HILTI_BUILDER_MODULE_H
#define HILTI_BUILDER_MODULE_H

#include "hilti-intern.h"

namespace hilti {
namespace builder {

class BlockBuilder;

/// Class for building a HILTI module.
class ModuleBuilder : public ast::Logger
{
public:
   /// Constructor setting up building a new module.
   ///
   /// id: The name of the module.
   ///
   /// path: A file system path associated with the module.
   ///
   /// libdirs: Directories where to search other modules when importing.
   ///
   /// l: A location associated with the module.
   ModuleBuilder(shared_ptr<hilti::ID> id, const std::string& path = "-", const path_list& libdirs = path_list(), const Location& l = Location::None);

   /// Constructor setting up building a new module.
   ///
   /// id: The name of the module.
   ///
   /// path: A file system path associated with the module.
   ///
   /// libdirs: Directories where to search other modules when importing.
   ///
   /// l: A location associated with the module.
   ModuleBuilder(const std::string& id, const std::string& path = "-", const path_list& libdirs = path_list(), const Location& l = Location::None);

   /// Destructor.
   ~ModuleBuilder();

   /// Finalizes building a module. By default, this also resolved and
   /// validates the module.
   ///
   /// resolve: If false, does not resolve the module.
   ///
   /// validate: If false, does not validate the module.
   ///
   /// Returns: True if everything succeeed. module() will then return the
   /// final module.
   ///
   /// \note If \a resolve is false, validation will almost certainly fail.
   bool finalize(bool resolve = true, bool validate = true);

   /// Prints out the module as HILTI source code. This must be called only after finalize.
   ///
   /// out: The stream where to print it to.
   void print(std::ostream& out);

   /// Imports another module into the current's namespace.
   ///
   /// module: The name of the module to import.
   void importModule(shared_ptr<ID> id);

   /// Imports another module into the current's namespace.
   ///
   /// module: The name of the module to import.
   void importModule(const std::string& id);

   /// Exports an ID so that it can be accessed from other modules.
   ///
   /// id: The id to export.
   void exportID(shared_ptr<ID> id);

   /// Exports an ID so that it can be accessed from other modules.
   ///
   /// id: The id to export.
   void exportID(const std::string& id);

   /// Exports a type, which means the compilter will generate externally
   /// visible type information for it.
   ///
   /// type: The type to export.
   void exportType(shared_ptr<hilti::Type> type);

   /// Returns the module's AST being built. Note that before calling
   /// finalize(), this remains in an unresolved state.
   shared_ptr<hilti::Module> module() const;

   /// Returns the current scope. This can be the global scope, a function
   /// scope, or a block scope, depending on the current state.
   shared_ptr<hilti::Scope> scope() const;

   /// Returns the top function form the stack of those currently being
   /// built.
   shared_ptr<hilti::Function> function() const;

   /// Returns the top builder on the current function's stack. If no
   /// function is current being built, return a builder for the module's
   /// global instruction block.
   shared_ptr<BlockBuilder> builder() const;

   /// Returns the block associated with the top builder on the current
   /// function's stack. If no function is current being built, returns the
   /// module's global instruction block.
   shared_ptr<hilti::expression::Block> block() const;

   /// Pushes a HILTI function onto the stack of functions currently being
   /// built. function() will now return this function. By default, the
   /// method then begins building the function's body and creates an initial
   /// block builder. builder() will now return that.
   ///
   /// func: The function to push.
   ///
   /// no_body: If true, don't push a body.
   ///
   /// Returns: An expression referencing the function.
   shared_ptr<hilti::declaration::Function> pushFunction(shared_ptr<hilti::Function> func, bool no_body = false);

   /// Adds a function declaration to the module, without beginning to build the function.
   ///
   /// func: The function to declare.
   ///
   /// Returns: An expression referencing the function.
   shared_ptr<hilti::declaration::Function> declareFunction(shared_ptr<hilti::Function> func);

   /// Adds a function declaration to the module, without beginning to build the function.
   ///
   ///
   /// Returns: An expression referencing the function.
   shared_ptr<hilti::declaration::Function> declareFunction(shared_ptr<ID> id,
                                                           shared_ptr<hilti::function::Result> result = nullptr,
                                                           const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                           hilti::type::function::CallingConvention cc = hilti::type::function::HILTI,
                                                           const Location& l=Location::None);

   /// Adds a function declaration to the module, without beginning to build the function.
   ///
   ///
   /// Returns: An expression referencing the function.
   shared_ptr<hilti::declaration::Function> declareFunction(const std::string& id,
                                                           shared_ptr<hilti::function::Result> result = nullptr,
                                                           const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                           hilti::type::function::CallingConvention cc = hilti::type::function::HILTI,
                                                           const Location& l=Location::None);

   /// Adds a hook declaration to the module, without beginning to build the hook.
   ///
   /// func: The hook to declare.
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> declareHook(shared_ptr<hilti::Hook> func);

   /// Adds a hook declaration to the module, without beginning to build the hook.
   ///
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> declareHook(shared_ptr<ID> id,
                                                       shared_ptr<hilti::function::Result> result = nullptr,
                                                       const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                       const Location& l=Location::None);

   /// Adds a hook declaration to the module, without beginning to build the hook.
   ///
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> declareHook(const std::string& id,
                                                       shared_ptr<hilti::function::Result> result = nullptr,
                                                       const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                       const Location& l=Location::None);

   /// Pushes a HILTI function onto the stack of functions currently being
   /// built. function() will now return this function. The method then
   /// begins building the function's body and creates an initial block
   /// builder. builder() will now return that.
   ///
   /// id: The name of the function.
   ///
   /// result: The parameter representing the return value. Can be null for a
   /// void function.
   ///
   /// params: The parameters to the functions.
   ///
   /// cc: The function's calling convention.
   ///
   /// scope: The function's scheduling scope.
   ///
   /// no_body: If true, don't push a body.
   ///
   /// l: Location associated with the type.
   ///
   /// Returns: An expression referencing the function.
   shared_ptr<hilti::declaration::Function> pushFunction(shared_ptr<ID> id,
                                                  shared_ptr<hilti::function::Result> result = nullptr,
                                                  const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                  hilti::type::function::CallingConvention cc = hilti::type::function::HILTI,
                                                  shared_ptr<Type> scope = nullptr,
                                                  bool no_body = false,
                                                  const Location& l=Location::None);

   /// Pushes a HILTI function onto the stack of functions currently being
   /// built. function() will now return this function. The method then
   /// begins building the function's body and creates an initial block
   /// builder. builder() will now return that.
   ///
   /// id: The name of the function.
   ///
   /// result: The parameter representing the return value. Can be null for a
   /// void function.
   ///
   /// params: The parameters to the functions.
   ///
   /// cc: The function's calling convention.
   ///
   /// scope: The function's scheduling scope.
   ///
   /// no_body: If true, don't push a body.
   ///
   /// l: Location associated with the type.
   ///
   /// Returns: An expression referencing the function.
   shared_ptr<hilti::declaration::Function> pushFunction(const std::string& id,
                                                  shared_ptr<hilti::function::Result> result = nullptr,
                                                  const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                                  hilti::type::function::CallingConvention cc = hilti::type::function::HILTI,
                                                  shared_ptr<Type> scope = nullptr,
                                                  bool no_body = false,
                                                  const Location& l=Location::None);

   /// Pops the top function from the stack of functions currently being
   /// built.
   shared_ptr<hilti::expression::Function> popFunction();

   /// Pushes a HILTI hook onto the stack of functions currently being built.
   /// Note that hooks and functions are internally kept on the same stack
   /// (as a hook is just a special form of a function). function() will now
   /// return this hook. The method then begins building the hook's body and
   /// creates an initial block builder. builder() will now return that.
   ///
   /// id: The name of the hook.
   ///
   /// result: The parameter representing the return value. Can be null for a
   /// void hook.
   ///
   /// params: The parameters to the hook.
   ///
   /// scope: The hook's scheduling scope.
   ///
   /// attrs: The hook attributes (i.e., priority and group).
   ///
   /// no_body: If true, don't push a body.
   ///
   /// l: Location associated with the type.
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> pushHook(shared_ptr<ID> id,
                                              shared_ptr<hilti::function::Result> result = nullptr,
                                              const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                              shared_ptr<Type> scope = nullptr,
                                              const hilti::hook::attribute_list& attrs = hilti::hook::attribute_list(),
                                              bool no_body = false,
                                              const Location& l=Location::None);

   /// Pushes a HILTI hook onto the stack of functions currently being built.
   /// Note that hooks and functions are internally kept on the same stack
   /// (as a hook is just a special form of a function). function() will now
   /// return this hook. The method then begins building the hook's body and
   /// creates an initial block builder. builder() will now return that.
   ///
   /// id: The name of the hook.
   ///
   /// result: The parameter representing the return value. Can be null for a
   /// void hook.
   ///
   /// params: The parameters to the hook.
   ///
   /// scope: The hook's scheduling scope.
   ///
   /// attrs: The hook attributes (i.e., priority and group).
   ///
   /// no_body: If true, don't push a body.
   ///
   /// l: Location associated with the type.
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> pushHook(const std::string& id,
                                              shared_ptr<hilti::function::Result> result = nullptr,
                                              const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                              shared_ptr<Type> scope = nullptr,
                                              const hilti::hook::attribute_list& attrs = hilti::hook::attribute_list(),
                                              bool no_body = false,
                                              const Location& l=Location::None);

   /// Pushes a HILTI hook onto the stack of functions currently being built.
   /// Note that hooks and functions are internally kept on the same stack
   /// (as a hook is just a special form of a function). function() will now
   /// return this hook. The method then begins building the hook's body and
   /// creates an initial block builder. builder() will now return that.
   ///
   /// id: The name of the hook.
   ///
   /// result: The parameter representing the return value. Can be null for a
   /// void hook.
   ///
   /// params: The parameters to the hook.
   ///
   /// scope: The hook's scheduling scope.
   ///
   /// priority: The hook's priority.
   ///
   /// group: The hook's group.
   ///
   /// l: Location associated with the type.
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> pushHook(shared_ptr<ID> id,
                                              shared_ptr<hilti::function::Result> result = nullptr,
                                              const hilti::function::parameter_list& params = hilti::function::parameter_list(),
                                              shared_ptr<Type> scope = nullptr,
                                              int64_t priority = 0, int64_t group = 0,
                                              bool no_body = false,
                                              const Location& l=Location::None);

   /// Pushes a HILTI hook onto the stack of functions currently being built.
   /// Note that hooks and functions are internally kept on the same stack
   /// (as a hook is just a special form of a function). function() will now
   /// return this hook. The method then begins building the hook's body and
   /// creates an initial block builder. builder() will now return that.
   ///
   /// id: The name of the hook.
   ///
   /// result: The parameter representing the return value. Can be null for a
   /// void hook.
   ///
   /// params: The parameters to the hook.
   ///
   /// scope: The hook's scheduling scope.
   ///
   /// priority: The hook's priority.
   ///
   /// group: The hook's group.
   ///
   /// l: Location associated with the type.
   ///
   /// Returns: An expression referencing the hook.
   shared_ptr<hilti::declaration::Function> pushHook(const std::string& id,
                                              shared_ptr<hilti::function::Result> result,
                                              const hilti::function::parameter_list& params,
                                              shared_ptr<Type> scope,
                                              int64_t priority, int64_t group,
                                              bool no_body = false,
                                              const Location& l=Location::None);

   /// Pops the top hook from the stack of functions currently being built.
   /// Note that hooks and functions are internally kept on the same stack
   /// (as a hook is just a special form of a function).
   shared_ptr<hilti::expression::Function> popHook();

   /// Pushes a new body onto the stack currently being built. A body is a
   /// series of blocks with a joint scope. The method also pushes an initial
   /// block builder on the body's stack. builder() will now return that.
   ///
   /// no_builder: If true, don't push a builder.
   ///
   /// l: An associated location.
   ///
   /// Returns: The also pushed builder, or null if no_builder is set.
   shared_ptr<BlockBuilder> pushBody(bool no_builder = false, const Location& l = Location::None);

   /// Pops athe top body from the stack currently being built.
   shared_ptr<hilti::expression::Block> popBody();

   /// Pushes a block builder onto the current body's stack of builders. This
   /// works like a sequence of newBuilder() and pushBuilder().
   ///
   /// name: The block's name. The method will always create a new block and
   /// adapt the name as necessary to make it unique.
   ///
   /// Returns: The pushed builder.
   shared_ptr<BlockBuilder> pushBuilder(shared_ptr<ID> id, const Location& l = Location::None);

   /// Pushes a block builder onto the current body's stack of builders. This
   /// works like a sequence of newBuilder() and pushBuilder().
   ///
   /// name: The block's name. The method will always create a new block and
   /// adapt the name as necessary to make it unique.
   ///
   /// Returns: The pushed builder.
   shared_ptr<BlockBuilder> pushBuilder(const std::string& id, const Location& l = Location::None);

   /// Pushes a block builder onto the current body's stack of builders.
   ///
   /// builder: The block builder to push.
   ///
   /// Returns: The pushed builder.
   shared_ptr<BlockBuilder> pushBuilder(shared_ptr<BlockBuilder> builder);

   /// Removes a previously pushed block builder from a function's stack,
   /// including everything on top of it.
   ///
   /// builder: The builder to remove with everythin on top of it.
   shared_ptr<BlockBuilder> popBuilder(shared_ptr<BlockBuilder> builder);

   /// Removes the top-most block builder from a function's stack.
   shared_ptr<BlockBuilder> popBuilder();

   /// Instantiates a new block builder.
   ///
   /// name: The block's name. The method will always create a new block and
   /// adapt the name as necessary to make it unique.
   ///
   /// Returns the new builder.
   shared_ptr<BlockBuilder> newBuilder(shared_ptr<ID> id, const Location& l = Location::None);

   /// Instantiates a new block builder.
   ///
   /// name: The block's name. The method will always create a new block and
   /// adapt the name as necessary to make it unique.
   ///
   /// Returns the new builder.
   shared_ptr<BlockBuilder> newBuilder(const std::string& id, const Location& l = Location::None);

   /// Adds a global variable to the module.
   ///
   /// id: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   ///
   /// force_unique: If true and an ID of the given name already exists,
   /// adapts the name to be unique. If false and the name is already taken,
   /// aborts with error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the global.
   shared_ptr<hilti::expression::Variable> addGlobal(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   /// Adds a global variable to the module.
   ///
   /// id: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   ///
   /// force_unique: If true and an ID of the given name already exists,
   /// adapts the name to be unique. If false and the name is already taken,
   /// aborts with error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the global.
   shared_ptr<hilti::expression::Variable> addGlobal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   /// Adds a global constant to the module.
   ///
   /// id: The name of the constant.
   ///
   /// type: The type of the constant.
   ///
   /// init: Value to initialize the constant. This must be of
   /// expression::Constant.
   ///
   /// force_unique: If true and an ID of the given name already exists,
   /// adapts the name to be unique. If false and the name is already taken,
   /// aborts with error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the global.
   shared_ptr<hilti::expression::Constant> addConstant(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique = false, const Location& l = Location::None);

   /// Adds a global constant to the module.
   ///
   /// id: The name of the constant.
   ///
   /// type: The type of the constant.
   ///
   /// init: Value to initialize the constant. This must be of
   /// expression::Constant.
   ///
   /// force_unique: If true and an ID of the given name already exists,
   /// adapts the name to be unique. If false and the name is already taken,
   /// aborts with error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the global.
   shared_ptr<hilti::expression::Constant> addConstant(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init, bool force_unique = false, const Location& l = Location::None);

   /// Adds a local variable to the current function.
   ///
   /// id: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the local with, or null for default.
   ///
   /// force_unique: If true and an ID of the given name already exists in
   /// the function's scope, adapts the name to be unique. If false and the
   /// name is already taken, aborts with error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the local.
   shared_ptr<hilti::expression::Variable> addLocal(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   /// Adds a local variable to the current function.
   ///
   /// id: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the local with, or null for default.
   ///
   /// force_unique: If true and an ID of the given name already exists in
   /// the function's scope, adapts the name to be unique. If false and the
   /// name is already taken, aborts with error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the local.
   shared_ptr<hilti::expression::Variable> addLocal(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   /// Adds a type declaration to the module.
   ///
   /// id: The name of the type. If the name is not unique, it will be made
   /// so.
   ///
   /// type: The type to associate with *name*.
   ///
   /// force_unique: If true and an ID of the given name already exists,
   /// adapts the name to be unique. If false and the name is already taken,
   /// aborts with an error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the type.
   shared_ptr<hilti::expression::Type> addType(shared_ptr<hilti::ID> id, shared_ptr<Type> type, bool force_unique = false, const Location& l = Location::None);

   /// Adds a type declaration to the module.
   ///
   /// id: The name of the type. If the name is not unique, it will be made
   /// so.
   ///
   /// type: The type to associate with *name*.
   ///
   /// force_unique: If true and an ID of the given name already exists,
   /// adapts the name to be unique. If false and the name is already taken,
   /// aborts with an error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the type.
   shared_ptr<hilti::expression::Type> addType(const std::string& id, shared_ptr<Type> type, bool force_unique = false, const Location& l = Location::None);

   /// Adds a context declaration to the module. Note that only a module must
   /// have at most one contecct declaration.
   ///
   /// type: The context of type type::Context.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the type.
   shared_ptr<hilti::expression::Type> addContext(shared_ptr<Type> type, const Location& l = Location::None);

   /// Adds a temporary variable to the current function.
   ///
   /// id: A name that will be used to build the name of the temporary
   /// variable. The methods always creates a new unique temporary and adapts
   /// the name as necessary (except for \a reuse, as below).
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   ///
   /// reuse: If true and a temporary of that name already exists, return
   /// that one. The types must be equivalent, if they don't, that's an
   /// error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the variable.
   shared_ptr<hilti::expression::Variable> addTmp(shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool reuse = false, const Location& l = Location::None);

   /// Adds a temporary variable to the current function.
   ///
   /// id: A name that will be used to build the name of the temporary
   /// variable. The methods always creates a new unique temporary and adapts
   /// the name as necessary (except for \a reuse, as below).
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   ///
   /// reuse: If true and a temporary of that name already exists, return
   /// that one. The types must be equivalent, if they don't, that's an
   /// error.
   ///
   /// l: An associated location.
   ///
   /// Returns: An expression referencing the variable.
   shared_ptr<hilti::expression::Variable> addTmp(const std::string& id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool reuse = false, const Location& l = Location::None);

   /// Caches an AST under a given key.
   ///
   /// component: A component name. The value is arbitrary, but the tuple (\a
   /// component, \a key) will be used to store the node.
   ///
   /// idx: An index name The value is arbitrary, but the tuple (\a component, \a
   /// key) will be used to store the node.
   ///
   /// node: The node to cache.
   void cacheNode(const std::string& component, const std::string& idx, shared_ptr<Node> node);

   /// Looks up a previously cached node.
   ///
   /// component: The component name passed to \a cacheNode.
   ///
   /// idx: The index name passed to \a cacheNode.
   ///
   /// Returns: The cached node, or null if none.
   shared_ptr<Node> lookupNode(const std::string& component, const std::string& idx);

protected:
   friend class BlockBuilder;

private:
   shared_ptr<hilti::expression::Variable> _addLocal(shared_ptr<statement::Block> stmt, shared_ptr<hilti::ID> id, shared_ptr<Type> type, shared_ptr<Expression> init = nullptr, bool force_unique = false, const Location& l = Location::None);

   typedef std::map<std::string, std::pair<std::string, shared_ptr<hilti::Declaration>>> declaration_map;

   enum _DeclStyle { REUSE, CHECK_UNIQUE, MAKE_UNIQUE };
   std::pair<shared_ptr<ID>, shared_ptr<Declaration>> _uniqueDecl(shared_ptr<ID> id, const std::string& kind, declaration_map* decls, _DeclStyle style, bool global);
   void _addDecl(shared_ptr<ID> id, const std::string& kind, declaration_map* decls, shared_ptr<Declaration>);

   struct Body {
       shared_ptr<statement::Block> stmt;
       shared_ptr<Scope> scope;
       std::list<shared_ptr<BlockBuilder>> builders;
   };

   struct Function {
       shared_ptr<hilti::Function> function;
       Location location;
       std::list<shared_ptr<Body>> bodies;
       declaration_map locals;
   };

   void Init(shared_ptr<ID> id, const std::string& path, const path_list& libdirs, const Location& l);
   shared_ptr<Function> _currentFunction() const;
   shared_ptr<Body> _currentBody() const;
   shared_ptr<BlockBuilder> _currentBuilder() const;

   shared_ptr<ID> _module_name;
   path_list _libdirs;
   bool _finalized = false;
   bool _correct = false;

   shared_ptr<Module> _module;
   std::list<shared_ptr<Function>> _functions;
   std::map<std::string, shared_ptr<Node>> _node_cache;
   declaration_map _globals;
};

}

}

#endif
