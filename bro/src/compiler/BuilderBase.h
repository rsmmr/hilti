///
/// Base class for functionality joined by all the builders.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_BUILDER_BASE_H
#define BRO_PLUGIN_HILTI_COMPILER_BUILDER_BASE_H

#include <stdexcept>
#include <string>
#include <memory>

class BroObj;
class BroType;
class Expr;
class Func;
class ID;
class Val;

namespace hilti {
	class Type;
	class Expression;

	namespace declaration {
		class Function;
	}

	namespace type {
		class Function;
        }

	namespace builder {
		class BlockBuilder;
		class ModuleBuilder;
	}
}

namespace plugin {
    namespace Bro_Hilti {
        class Plugin;
    }
}

extern plugin::Bro_Hilti::Plugin HiltiPlugin;

namespace bro {
namespace hilti {

class ValueConverter;

namespace compiler {

#ifdef DEBUG
#define DBG_LOG_COMPILER(args...) debug_logger.Log((::plugin::Plugin&)HiltiPlugin, args)
#else
#define DBG_LOG_COMPILER(args...)
#endif

class BuilderException : public std::runtime_error {
public:
	BuilderException(const std::string& what) : std::runtime_error(what)	{}
};

class ModuleBuilder;

class BuilderBase  {
public:
	/**
	 * Returns the module builder currently in use.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 */
	::hilti::builder::ModuleBuilder* ModuleBuilder();

	/**
	 * Returns the current block builder.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 *
	 * @return The block builder.
	 */
	std::shared_ptr<::hilti::builder::BlockBuilder> Builder() const;

	/**
	 * Returns the module builder for the glue code module.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 */
	::hilti::builder::ModuleBuilder* GlueModuleBuilder() const;

	/**
	 * Throws a \a BuilderException, terminating the current compilation.
	 *
	 * msg: The error message to carry upstream with the exception
	 */
	void Error(const std::string& msg, const BroObj* obj = 0) const;

	/**
	 * Logs a warning.
	 *
	 * msg: The warning message to log.
	 */
	void Warning(const std::string& msg, const BroObj* obj = 0) const;

	/**
	 * Reports an internal error and aborts execution.
	 *
	 * msg: The error message to log.
	 */
	void InternalError(const std::string& msg, const BroObj* obj = 0) const;

	/**
	 * Returns the compiler in use. This forwards to the corresponding
	 * method of the ModuleBuilder.
	 */
	class Compiler* Compiler() const;

	/**
	 * Returns the expression builder to use. This forwards to the
	 * corresponding method of the ModuleBuilder.
	 */
	class ExpressionBuilder* ExpressionBuilder() const;

	/**
	 * Returns the statement builder to use. This forwards to the
	 * corresponding method of the ModuleBuilder.
	 */
	class StatementBuilder* StatementBuilder() const;

	/**
	 * Returns the type builder to use. This forwards to the
	 * corresponding method of the ModuleBuilder.
	 */
	class TypeBuilder* TypeBuilder() const;

	/**
	 * Returns the value builder to use. This forwards to the
	 * corresponding method of the ModuleBuilder.
	 */
	class ValueBuilder* ValueBuilder() const;

	/**
	 * Returns the conversion builder to use. This forwards to the
	 * corresponding method of the ModuleBuilder.
	 */
	class ValueConverter* ValueConverter() const;

	/**
	 * Returns the internal HILTI-level symbol for a Bro Function.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 *
	 * @param func The function to return the symbol for.
	 */
	std::string HiltiSymbol(const ::Func* func, bool include_module = false);

	/**
	 * Returns the internal HILTI-level symbol for the stub of a Bro
	 * Function.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 *
	 * @param func The function to return the symbol for.
	 *
	 * @param include_module If true, the returned name will include the
	 * module name and hence represent the symbol as visibile at the LLVM
	 * level after linking.
	 */
	std::string HiltiStubSymbol(const ::Func* func, bool include_module);

	/**
	 * Returns the internal HILTI-level symbol for a Bro ID.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 */
	std::string HiltiSymbol(const ::ID* id);

	/**
	 * Returns the internal HILTI-level symbol for a Bro type. This will
	 * always be a globally valid ID.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 */
	std::string HiltiSymbol(const ::BroType* t);

	/**
	 * Renders a \a BroObj via its \c Describe() method and turns the
	 * result into a valid HILTI identifier string.
	 *
	 * This is a short-cut to using the compiler's corresponding method.
	 *
	 * @param obj The object to describe.
	 *
	 * @return A string with a valid HILTI identifier.
	 */
	std::string HiltiODescSymbol(const ::BroObj* obj);

	/**
	 * Converts a Bro type into its HILTI equivalent. This is a short-cut
	 * to using the type builder's corresponding Compile() method.
	 *
	 * @param type The Bro type to converts.
	 *
	 * @return The corresponding HILTI type.
	 */
	std::shared_ptr<::hilti::Type> HiltiType(const ::BroType* type);

    	/**
	 * Returns the HILTI type for a Bro function. This may be different
	 * than what Compile() returns: while the Compile() returns how we
	 * represent a variable of the function's type, this returns the type
	 * of the function itself.
	 *
	 * @param type The Bro function type.
	 *
	 * @return The corresponding HILTI type.
	 */
	std::shared_ptr<::hilti::type::Function> HiltiFunctionType(const ::FuncType* type);

	/**
	 * Compiles a Bro value into its HILTI expression equivalent. This is
	 * a short-cut to using the value builder's corresponding Compile()
	 * method.
	 *
	 * @param val The Bro value to compile.
	 *
	 * @param target_type The target type that the compiled value should
	 * have. This acts primarily as a hint in case the expression's type
	 * isn't unambigious (e.g., with untyped constructors).
	 *
	 * @param init If true, the converted value will be used to
	 * initialize a variable or struct field. A type may want to change
	 * what it returns in initialization contexts. 
	 *
	 * @return The corresponding HILTI expression.
	 */
	std::shared_ptr<::hilti::Expression> HiltiValue(const ::Val* val,
							const ::BroType* target_type = nullptr,
							bool init = false);

	/**
	 * Returns the default initialization value for variables of a given
	 * Bro type.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to initialize variables with, or null
	 * for HILTI's default init value.
	 */
	shared_ptr<::hilti::Expression> HiltiDefaultInitValue(const ::BroType* type);

	/**
	 * Returns a HILTI expression with a Bro value refering to a BroType.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to refer to the type, of type
	 * LibBro::BroVal.
	 */
	shared_ptr<::hilti::Expression> HiltiBroVal(const ::BroType* type);

	/**
	 * Returns a HILTI expression referencing a BroType.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to refer to the type, of type LibBro::BroType.
	 */
	shared_ptr<::hilti::Expression> HiltiBroType(const ::BroType* type);

	/**
	 * Returns a HILTI expression with a Bro value refering to a Bro
	 * Func.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to refer to the type, of type
	 * LibBro::BroVal.
	 */
	shared_ptr<::hilti::Expression> HiltiBroVal(const ::Func* type);

	/**
	 * Compiles a Bro expression into its HILTI equivalent. This is a
	 * short-cut to using the expression's builder's corresponding
	 * Compile() method.
	 *
	 * @param expr The Bro expression to compile.
	 *
	 * @param target_type The target type that the compiled expression
	 * should have. This acts primarily as a hint in case the
	 * expression's type isn't unambigious (e.g., with untyped
	 * constructors).
	 *
	 * @return The corresponding HILTI expression.
	 */
	std::shared_ptr<::hilti::Expression> HiltiExpression(const ::Expr* expr,
							     const ::BroType* target_type = nullptr);

	/**
	 * Turns a Bro expression that's used as a table or vector index into the
	 * corresponding HILTI value. This takes care of treating single-element
	 * ListVals correctly (i.e., by using that element directly).
	 *
	 * @param idx The Bro expression referencing the index value.
	 *
	 * @return The HILTI value to be used with map/set/vector instructions.
	 */
	std::shared_ptr<::hilti::Expression> HiltiIndex(const ::Expr* idx);

	/**
	 * Turns a Bro record field name into the corresponding HILTI value
	 * for use with a \c struct instruction.
	 *
	 * @param fname The Bro field name.
	 *
	 * @return The HILTI value to be used with \c struct instructions.
	 */
	std::shared_ptr<::hilti::Expression> HiltiStructField(const char* fname);

	/**
	 * Conerts a Bro \a Val into a HILTI expression dynamically at
	 * runtime.  This is a short-cut to using the conversion builder's
	 * corresponding Convert() method.
	 *
	 * @param val The value to convert.
	 *
	 * @return An expression referencing the converted value.
	 */
	std::shared_ptr<::hilti::Expression> RuntimeValToHilti(std::shared_ptr<::hilti::Expression> val, const ::BroType* type);

	/**
	 * Conerts a HILTI expression into a Bro \a Val dynamically at
	 * runtime.  This is a short-cut to using the conversion builder's
	 * corresponding Convert() method.
	 *
	 * @param val The value to convert.
	 *
	 * @param type The target Bro type to convert into.
	 *
	 * @return An expression referencing the converted value, which
	 * correspond to a pointer to a Bro Val.
	 */
	std::shared_ptr<::hilti::Expression> RuntimeHiltiToVal(std::shared_ptr<::hilti::Expression> val, const ::BroType* type);

	/**
	 * Declares the prototype for single function. This branches out into
	 * the other Declare*() methods for the various function flavors.
	 *
	 * If the declaration already exists, that one is returned.
	 *
	 * The method forwards to ModuleBuilder::DeclareFunction.
	 *
	 * @param func The function to compile.
	 *
	 * @return An expression referencing the declared function.
	 */
	std::shared_ptr<::hilti::Expression> DeclareFunction(const ::Func* func);

	/**
	 * Declares a global variable. If the declaration already
	 * exists, that one is returned.
	 *
	 * The method forwards to ModuleBuilder::DeclareGlobal.
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
	 * The method forwards to ModuleBuilder::DeclareLocal.
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
	 * Attempts to statically determine the Bro function referenced by a
	 * Bro expression. This may or may not work.
	 *
	 * The method forwards to Compiler::BroExprToFunc.
	 *
	 * @param func The expression referencing a function
	 *
	 * @return A pair \a (success, function). If \a success is true, we
	 * could infer which Bro function the expression referes to; in that
	 * case, the function is usually it in the 2nd pair element if
	 * there's a local implementation (and null instead if not). If \a
	 * success is false, we couldn't statically determine behind the
	 * expression; in that case, the 2nd pair element is undefined.
	 */
	std::pair<bool, ::Func*> BroExprToFunc(const ::Expr* func);

	/**
	 * XXX
	 */
	shared_ptr<::hilti::Expression> HiltiCallFunction(const ::Expr* func, ::FuncType* ftype, ListExpr* args);

	/**
	 * XXXX
	 */
	void MapType(const ::BroType* from, const ::BroType* to);

	/**
	 * Returns a string with location information for a BroObj.
	 */
	std::string Location(const ::BroObj *obj) const;

protected:
	/**
	 * Constructor.
	 *
	 * mbuilder: The module builder to use.
	 */
	BuilderBase(class ModuleBuilder* mbuilder);

private:
	class ModuleBuilder* mbuilder;
};

}
}
}

#endif


