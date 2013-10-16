///
/// A ModuleBuilder compiles a complete Bro namespace into a HILTI module.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_MODULE_BUILDER_H
# define BRO_PLUGIN_HILTI_COMPILER_MODULE_BUILDER_H

#include <hilti/builder/module.h>

#include "BuilderBase.h"

class Func;
class BroFunc;
class BuiltinFunc;

namespace bro {
namespace hilti {
namespace compiler {

class Compiler;
class ModuleBuilderCallback;

class ModuleBuilder : public BuilderBase, public ::hilti::builder::ModuleBuilder {
public:
	using BuilderBase::Error;
	using BuilderBase::Warning;

	/**
	 * Constructor.
	 *
	 * @param compiler The compiler that instantiated this builder.
	 *
	 * @param ctx The global HILTI compiler context to use for building
	 * the module.
	 *
	 * @param namespace: The Bro namespace to compile.
	 */
	ModuleBuilder(class Compiler* compiler, shared_ptr<::hilti::CompilerContext> ctx, const std::string& namespace_);

	/**
	 * Destructor.
	 */
	~ModuleBuilder();

	/**
	 * Compiles the namespace into HILTI code.
	 *
	 * @return The compiled module, or null on error. In the error cases,
	 * suitable error messages have already been reported. */
	shared_ptr<::hilti::Module> Compile();

	/**
	 * Returns the compiler that instantiated this builder.
	 */
	class Compiler* Compiler() const;

	/**
	 * Returns the expression builder to use with this module.
	 */
	class ExpressionBuilder* ExpressionBuilder() const;

	/**
	 * Returns the statement builder to use with this module.
	 */
	class StatementBuilder* StatementBuilder() const;

	/**
	 * Returns the type builder to use with this module.
	 */
	class TypeBuilder* TypeBuilder() const;

	/**
	 * Returns the value builder to use with this module.
	 */
	class ValueBuilder* ValueBuilder() const;

	/**
	 * Returns the coversion builder to use with this module.
	 */
	class ConversionBuilder* ConversionBuilder() const;

	/**
	 * Returns the namespace being compiled.
	 */
	string Namespace() const;

	/**
	 * Declares the prototype for single function. This branches out into
	 * the other Declare*() methods for the various function flavors.
	 * 
	 * If the declaration already exists, that one is returned.
	 *
	 * @param func The function to compile.
	 *
	 * @return The declaration.
	 */
	shared_ptr<::hilti::declaration::Function> DeclareFunction(const ::Func* func);

protected:
	friend class ModuleBuilderCallback;

	/**
	 * Compiles a single function. This branches out into the other
	 * Compile*() methods for the various function flavors.
	 *
	 * @param func The function to compile.
	 */
	void CompileFunction(const ::Func* func);

	/**
	 * Compiles a script function's body.
	 *
	 * @param func The func to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_FUNCTION. */
	void CompileScriptFunction(const ::BroFunc* func);

	/**
	 * Compiles all the bodies for an event.
	 *
	 * @param event The event to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_EVENT.
	 */
	void CompileEvent(const ::BroFunc* event);

	/**
	 * Compiles all the bodies for a hook.
	 *
	 * @param hook The hook to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_HOOK.
	 */
	void CompileHook(const ::BroFunc* event);

	/**
	 * Compiles a function body. The function itself must have already
	 * been generated and pushed on top of the builder stack.
	 *
	 * @param func The current function.
	 *
	 * @param body The body to compile. This must be a pointer to a
	 * ::Func::Body; however we use void here to avoid having to include
	 * the Bro header file.
	 */
	void CompileFunctionBody(const ::Func* func, void* body);

	/**
	 * Declares the prototype for a script function.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param func The func to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_FUNCTION.
	 *
	 * @return The declaration.
	 */
	shared_ptr<::hilti::declaration::Function> DeclareScriptFunction(const ::BroFunc* func);

	/**
	 * Declares the prorotype for an event.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param event The event to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_EVENT.
	 *
	 * @return The declaration.
	 */
	shared_ptr<::hilti::declaration::Function> DeclareEvent(const ::BroFunc* event);

	/**
	 * Declares the prorotype for a hook.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param hook The hook to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_HOOK.
	 *
	 * @return The declaration.
	 */
	shared_ptr<::hilti::declaration::Function> DeclareHook(const ::BroFunc* event);

	/**
	 * Declares the prorotype for a built-in function.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param func The func to compile.
	 *
	 * @return The declaration.
	 */
	shared_ptr<::hilti::declaration::Function> DeclareBuiltinFunction(const ::BuiltinFunc* bif);

private:
	string ns;

	class Compiler* compiler;
	class ExpressionBuilder* expression_builder;
	class StatementBuilder* statement_builder;
	class TypeBuilder* type_builder;
	class ValueBuilder* value_builder;
	class ConversionBuilder* conversion_builder;

	static std::shared_ptr<ModuleBuilderCallback> callback;
};

}
}
}

#endif
