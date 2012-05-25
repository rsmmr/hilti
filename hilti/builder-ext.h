
#ifndef HILTI_BUILDER_EXT_H
#define HILTI_BUILDER_EXT_H

#include "hilti.h"

namespace hilti {

namespace builder {

class ModuleBuilder
{
public:
   /// Constructor. Begins building a new HILTI module.
   ///
   /// name: The module name.
   ModuleBuilder(const string& name);

   /// Returns the module being built.
   shared_ptr<Module> module();

   /// Adds a global variable to the module.
   ///
   /// name: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// init: Value to initialize the global with, or null for default.
   void addGlobal(const string& name, shared_ptr<Type> type, shared_ptr<Expr> init = nullptr);

   /// Adds a global constant to the module.
   ///
   /// name: The name of the variable.
   ///
   /// type: The type of the variable.
   ///
   /// value: The constant's value.
   void addConstant(const string& name, shared_ptr<Type> type, shared_ptr<Expr> value);

   /// Adds a type declaration to the module.
   ///
   /// name: The name of the type.
   ///
   /// type: The type to associate with *name*.
   void addType(const string& name, shared_ptr<Type> type);

   /// Imports another module into the current's namespace.
   ///
   /// module: The name of the module to import.
   void importModule(const string& name);

   /// Returns a type corresponding to the name.
   ///
   /// name: The name of the type to return.
   ///
   /// Returns: The type, which may still be unresolved.
   shared_ptr<Type> lookupType(const string& name);

   /// Starts building a new function and pushes it on top of stack of
   /// functions being built.
   ///
   /// name: The name of the function.
   ///
   /// push_block_builder: If true, an initial BlockBuilder is added to the
   /// function as its entry block.
   ///
   /// hook: If true, a hook function will be created.
   shared_ptr<FunctionBuilder> pushFunction(const string& name, bool push_block_builder = true, bool hook = false);

   /// Returns the function on top of the stack currently being built.
   shared_ptr<FunctionBuilder> function() const;

   /// Finishes building the function on the top of the function stack. Pops it from the stack.
   void popFunction();

};

class FunctionBuilder
{
public:
   moduleBuilder
   function
   addLocal
   addTmp
   newBlock
   pushBlock
   popBlock
};

class BlockBuilder
{
public:
   block
   functionBuilder
   addLocal
   addTmp
   terminated
   setBlockComment
   setInstructionComment
   createInstruction
   createIf
   createIfElse
   createSelect
   createSwitch
   createTryCatch
   createRaise
   createDebugMsg
   createInternalError
   createFor

};

}

}
