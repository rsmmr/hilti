///
/// Bro script compiler providing the main entry point to compiling a
/// complete Bro configuration into HILTI.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_COMPILER_H
#define BRO_PLUGIN_HILTI_COMPILER_COMPILER_H

#include <list>
#include <set>
#include <memory>

class Func;
class ID;

namespace hilti {
	class CompilerContext;
	class Module;

	namespace builder {
		class BlockBuilder;
		class ModuleBuilder;
	}
}

namespace bro {
namespace hilti {

namespace compiler {

class Compiler {
public:
	typedef std::list<std::shared_ptr<::hilti::Module>> module_list;

	/**
	 * Constructor.
	 *
	 * ctx: The HILTI context to use for compiling script code. */
	Compiler(std::shared_ptr<::hilti::CompilerContext> ctx);

	/**
	 * Destructor.
	 */
	~Compiler();

	/**
	 * Compiles all of Bro's script code.
	 *
	 * @return A list of compiled modules.
	 */
	module_list CompileAll();

	/**
	 * Returns the module builder currently in use.
	 */
	::hilti::builder::ModuleBuilder* ModuleBuilder();

	/**
	 * Returns the current block builder. This is a short-cut to calling
	 * the current corresponding method of the current module builder.
	 *
	 * @return The block builder.
	 */
	std::shared_ptr<::hilti::builder::BlockBuilder> Builder() const;

	/**
	 * Pushes another HILTI module builder on top of the current stack.
	 * This one will now be consulted by Builder().
	 */
	void pushModuleBuilder(::hilti::builder::ModuleBuilder* mbuilder);

	/**
	 * Removes the top-most HILTI module builder from the stack. This
	 * must not be called more often than pushModuleBuilder().
	 */
	void popModuleBuilder();

	/**
	 * Returns the module builder for the glue code module.
	 */
	::hilti::builder::ModuleBuilder* GlueModuleBuilder() const;

	/**
	 * Returns the internal HILTI-level symbol for a Bro Function.
	 *
	 * @param func The function.
	 *
	 * @param module If non-null, a module to which the returned symbol
	 * should be relative. If the function's ID has the same namespace as
	 * the module, it will be skipped; otherwise included.
	 */
	std::string HiltiSymbol(const ::Func* func, shared_ptr<::hilti::Module> module, bool include_module = false);

	/**
	 * Returns the internal HILTI-level symbol for the stub of a Bro Function.
	 *
	 * @param func The function.
	 *
	 * @param module If non-null, a module to which the returned symbol
	 * should be relative. If the function's ID has the same namespace as
	 * the module, it will be skipped; otherwise included.
	 *
	 * @param include_module If true, the returned name will include the
	 * module name and hence reoresent the symbol as visibile at the LLVM
	 * level after linking.
	 */
	std::string HiltiStubSymbol(const ::Func* func, shared_ptr<::hilti::Module> module, bool include_module);

	/**
	 * Returns the internal HILTI-level symbol for a Bro ID.
	 *
	 * @param id The ID.
	 *
	 * @param module: If non-null, a module to which the returned symbol
	 * should be relative. If the function's ID has the same namespace as
	 * the module, it will be skipped; otherwise included.
	 */
	std::string HiltiSymbol(const ::ID* id, shared_ptr<::hilti::Module> module);

	/**
	 * Returns the internal HILTI-level symbol for a Bro ID.
	 *
	 * @param id The ID.
     *
     * @param global True if this is a global ID that need potentially needs
     * to be qualified with a namespace.
	 *
	 * @param module: If non-empty, a module name to which the returned
	 * symbol should be relative. If the function's ID has the same
	 * namespace as the module, it will be skipped; otherwise included.
	 *
	 * @param include_module If true, the returned name will include the
	 * module name and hence reoresent the symbol as visibile at the LLVM
	 * level after linking.
	 */
	std::string HiltiSymbol(const std::string& id, bool global, const std::string& module, bool include_module = false);

	/**
	 * Renders a \a BroObj via its \c Describe() method and turns the
	 * result into a valid HILTI identifier string.
	 *
	 * @param obj The object to describe.
	 *
	 * @return A string with a valid HILTI identifier.
	 */
	std::string HiltiODescSymbol(const ::BroObj* obj);

	/**
	 * Checks if a built-in Bro function is available at HILTI-level.
	 *
	 * @param The fully-qualified Bro-level name of the function.
	 *
	 * @param If given, the HILTI-level name of the function will be
	 * stored in here.
	 *
	 * @param True if the BiF is available at the HILTI-level.
	 */
	bool HaveHiltiBif(const std::string& name, std::string* hilti_name = 0);

private:
	std::string normalizeSymbol(const std::string sym, const std::string prefix, const std::string postfix, const std::string& module, bool global, bool include_module = false);

	std::list<std::string> GetNamespaces() const;

	std::shared_ptr<::hilti::CompilerContext> hilti_context;

	typedef std::list<::hilti::builder::ModuleBuilder*> module_builder_list;
	module_builder_list mbuilders;

	shared_ptr<::hilti::builder::ModuleBuilder> glue_module_builder;
};

}
}
}

#endif
