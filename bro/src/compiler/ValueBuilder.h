///
/// A ValueBuilder converts a Bro \a Val into a corresponding HILTI
/// expression.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_VALUE_BUILDER_H
#define BRO_PLUGIN_HILTI_COMPILER_VALUE_BUILDER_H

#include "BuilderBase.h"

class AddrVal;
class EnumVal;
class ListVal;
class OpaqueVal;
class PatternVal;
class PortVal;
class RecordVal;
class StringVal;
class SubNetVal;
class TableVal;
class Val;
class VectorVal;

namespace hilti { class Expression; }

namespace bro {
namespace hilti {
namespace compiler {

class ValueBuilder : public BuilderBase {
public:
	/**
	 * Constructor.
	 *
	 * mbuilder: The module builder to use.
	 */
	ValueBuilder(class ModuleBuilder* mbuilder);

	/**
	 * Converts a Bro \a Val into its HILTI equivalent.
	 *
	 * @param val The value to compile.
	 *
	 * @param target_type The target type that the compiled value should
	 * have. This acts primarily as a hint in case the expression's type
	 * isn't unambigious (e.g., with untyped constructors).
	 *
	 * @return The converted expression.
	 */
	shared_ptr<::hilti::Expression> Compile(const ::Val* val,
						shared_ptr<::hilti::Type> target_type = nullptr);

	/**
	 * Returns the default initialization value for variables of a given
	 * Bro type.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to initialize variables with, or null
	 * for HILTI's default init value.
	 */
	shared_ptr<::hilti::Expression> InitValue(const ::BroType* type);

protected:
	/**
	 * Returns the target type passed into Compile(), or null if none.
	 */
	shared_ptr<::hilti::Type> TargetType() const;

	std::shared_ptr<::hilti::Expression> CompileBaseVal(const ::Val* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::AddrVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::EnumVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::ListVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::OpaqueVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::PatternVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::PortVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::RecordVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::StringVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::SubNetVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::TableVal* val);
	std::shared_ptr<::hilti::Expression> Compile(const ::VectorVal* val);

private:
	std::list<shared_ptr<::hilti::Type>> target_types;
};

}
}
}

#endif
