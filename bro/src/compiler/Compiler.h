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

private:
	std::string normalizeSymbol(const std::string sym, const std::string prefix, const std::string postfix, shared_ptr<::hilti::Module> module, bool include_module = false);

	std::list<std::string> GetNamespaces() const;

	std::shared_ptr<::hilti::CompilerContext> hilti_context;

};

}
}
}

#endif
