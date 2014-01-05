///
/// A TypeBuilder converts a Bro type into its HILTI equivalent.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_TYPE_BUILDER_H
#define BRO_PLUGIN_HILTI_COMPILER_TYPE_BUILDER_H

#include "BuilderBase.h"

class BroType;
class EnumType;
class FileType;
class FuncType;
class OpaqueType;
class RecordType;
class SubNetType;
class TableType;
class TypeList;
class TypeType;
class VectorType;

namespace hilti { class Type; }

namespace bro {
namespace hilti {
namespace compiler {

class TypeBuilder : public BuilderBase {
public:
	/**
	 * Constructor.
	 *
	 * mbuilder: The module builder to use.
	 */
	TypeBuilder(class ModuleBuilder* mbuilder);

	/**
	 * Converts a Bro type into its HILTI equivalent.
	 *
	 * @return The converted type
	 */
	shared_ptr<::hilti::Type> Compile(const ::BroType* type);

	/**
	 * Returns the HILTI type for a Bro function. This may be different
	 * than what Compile() returns: while the Compile() returns how we
	 * represent a variable of the function's type, this returns the type
	 * of the function itself.
	 */
	shared_ptr<::hilti::type::Function> FunctionType(const ::FuncType* ftype);

protected:
	std::shared_ptr<::hilti::Type> CompileBaseType(const ::BroType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::EnumType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::FileType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::FuncType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::OpaqueType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::RecordType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::SubNetType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::TableType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::TypeList* type);
	std::shared_ptr<::hilti::Type> Compile(const ::TypeType* type);
	std::shared_ptr<::hilti::Type> Compile(const ::VectorType* type);
};

}
}
}

#endif
