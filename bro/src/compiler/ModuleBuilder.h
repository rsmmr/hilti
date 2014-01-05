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
	 * @return An expression referencing the declared function.
	 */
	shared_ptr<::hilti::Expression> DeclareFunction(const Func* func);

	/**
	 * Declares a global variable. If the declaration already
	 * exists, that one is returned.
	 *
	 * @param id The Bro ID to declare a corresponding global for.
	 *
	 * @return An expression referencing the declared global.
	 */
	std::shared_ptr<::hilti::Expression> DeclareGlobal(const ::ID* id);

	/**
	 * Declares a local variable. If the declaration already
	 * exists, that one is returned.
	 *
	 * @param id The Bro ID to declare a corresponding local for.
	 *
	 * @return An expression referencing the declared local.
	 */
	std::shared_ptr<::hilti::Expression> DeclareLocal(const ::ID* id);

	/**
	 * XXX
	 */
	const ::Func* CurrentFunction() const;

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallFunction(const ::Expr* func, const ::FuncType* ftype, ListExpr* args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallFunctionViaBro(shared_ptr<::hilti::Expression> func, const ::FuncType* ftype, ListExpr* args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallScriptFunctionHilti(const ::Func* func, shared_ptr<::hilti::Expression> args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallEventHilti(const ::Func* func, shared_ptr<::hilti::Expression> args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallHookHilti(const ::Func* func, shared_ptr<::hilti::Expression> args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallBuiltinFunctionHilti(const ::Func* func, shared_ptr<::hilti::Expression> args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallEventLegacy(shared_ptr<::hilti::Expression> func, const ::FuncType* ftype, ListExpr* args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallHookLegacy(shared_ptr<::hilti::Expression> func, const ::FuncType* ftype, ListExpr* args);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallFunctionLegacy(shared_ptr<::hilti::Expression> fval, const ::FuncType* ftype, ListExpr* args);

	/**
	 * XXX
	 */
	void FinalizeGlueCode();

protected:
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
	void CompileScriptFunction(const ::BroFunc* func, bool exported);

	/**
	 * Compiles all the bodies for an event.
	 *
	 * @param event The event to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_EVENT.
	 */
	void CompileEvent(const ::BroFunc* event, bool exported);

	/**
	 * Compiles all the bodies for a hook.
	 *
	 * @param hook The hook to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_HOOK.
	 */
	void CompileHook(const ::BroFunc* event, bool exported);

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
	 * @return An expression referencing the declared function.
	 */
	shared_ptr<::hilti::Expression> DeclareScriptFunction(const ::BroFunc* func);

	/**
	 * Declares the prorotype for an event.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param event The event to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_EVENT.
	 *
	 * @return An expression referencing the declared function.
	 */
	shared_ptr<::hilti::Expression> DeclareEvent(const ::BroFunc* event);

	/**
	 * Declares the prorotype for a hook.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param hook The hook to compile. The function type's flavor must
	 * be \c FUNC_FLAVOR_HOOK.
	 *
	 * @return An expression referencing the declared function.
	 */
	shared_ptr<::hilti::Expression> DeclareHook(const ::BroFunc* event);

	/**
	 * Declares the prorotype for a built-in function.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * @param func The func to compile.
	 *
	 * @return An expression referencing the declared function.
	 */
	shared_ptr<::hilti::Expression> DeclareBuiltinFunction(const ::BuiltinFunc* bif);

	/**
	 * XXX
	 */
	typedef std::function<void (ModuleBuilder*,
				    shared_ptr<::hilti::Expression> rval,
				    shared_ptr<::hilti::Expression>)>
		create_stub_callback;

	void CreateFunctionStub(const BroFunc* func, create_stub_callback cb);

private:
	string ns;
	std::list<const ::Func*> functions;

	class Compiler* compiler;
	class ExpressionBuilder* expression_builder;
	class StatementBuilder* statement_builder;
	class TypeBuilder* type_builder;
	class ValueBuilder* value_builder;
	class ConversionBuilder* conversion_builder;
};

}
}
}

#endif
