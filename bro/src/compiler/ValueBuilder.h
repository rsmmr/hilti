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
	 * @param init If true, the converted value will be used to
	 * initialize a variable or struct field. A type may want to change
	 * what it returns in initialization contexts. 
	 *
	 * @return The converted expression.
	 */
	shared_ptr<::hilti::Expression> Compile(const ::Val* val,
						const ::BroType* target_type = nullptr, bool init = false);

	/**
	 * Returns the default initialization value for variables of a given
	 * Bro type.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to initialize variables with, or null
	 * for HILTI's default init value.
	 */
	shared_ptr<::hilti::Expression> DefaultInitValue(const ::BroType* type);

	/**
	 * Returns a HILTI expression with a Bro value refering to a BroType.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to refer to the type, of type
	 * LibBro::BroVal.
	 */
	shared_ptr<::hilti::Expression> TypeVal(const ::BroType* type);

	/**
	 * Returns a HILTI expression referencing a BroType.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to refer to the type, of type LibBro::BroType.
	 */
	shared_ptr<::hilti::Expression> BroType(const ::BroType* type);

	/**
	 * Returns a HILTI expression with a Bro value refering to a Bro
	 * Func.
	 *
	 * @param type The Bro type.
	 *
	 * @return An HILTI expression to refer to the type, of type
	 * LibBro::BroVal.
	 */
	shared_ptr<::hilti::Expression> FunctionVal(const ::Func* type);

protected:
	/** 
	 * Returns the target type passed into Compile(). This will abort if
	 * not target type has been set for the current value.
	 */
	const ::BroType* TargetType() const;

	/**
	 * Returns true if a target type has been set for the current value.
	 */
	bool HasTargetType() const;

	/**
	 * Returns true if the converted value will be used in an
	 * initialization context. This reflects the flag passed into
	 * Compile().
	 */
	bool IsInit() const;

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
	std::list<const ::BroType *> target_types;
	bool is_init;
};

}
}
}

#endif
