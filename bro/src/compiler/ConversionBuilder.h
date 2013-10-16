///
/// A ConversionBuilder generates code to converte between a Bro \a Vals and
/// HILTI expressions at runtime.
///

#ifndef BRO_PLUGIN_HILTI_COMPILER_CONVERSION_BUILDER_H
#define BRO_PLUGIN_HILTI_COMPILER_CONVERSION_BUILDER_H

#include "BuilderBase.h"

class AddrType;
class EnumType;
class TypeList;
class OpaqueType;
class RecordType;
class SubNetType;
class TableType;
class TypeType;
class VectorType;

namespace hilti {
	class Expression;
	class Type;
}

namespace bro {
namespace hilti {
namespace compiler {

class ConversionBuilder : public BuilderBase {
public:
	using BuilderBase::Error;

	/**
	 * Constructor.
	 *
	 * mbuilder: The module builder to use.
	 */
	ConversionBuilder(class ModuleBuilder* mbuilder);

	/**
	 * Generates code to convert a Bro \a Val into its HILTI equivalent
	 * at runtime.
	 *
	 * @param val The value to convert.
	 *
	 * @return An expression referencing the converted value.
	 */
	shared_ptr<::hilti::Expression> ConvertBroToHilti(shared_ptr<::hilti::Expression> val, const ::BroType* type);

	/**
	 * Generates code to convert a HILTI expression into a corresponding
	 * Bro \a Val at runtime.
	 *
	 * @param val The value to convert.
	 *
	 * @param type The target Bro type to convert into.
	 *
	 * @return An expression referencing the converted value, which
	 * correspond to a pointer to a Bro Val.
	 */
	shared_ptr<::hilti::Expression> ConvertHiltiToBro(shared_ptr<::hilti::Expression> val, const ::BroType* type);

protected:
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::BroType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::AddrType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::EnumType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::TypeList* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::OpaqueType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::RecordType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::SubNetType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::TableType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::TypeType* type);
	std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val, const ::VectorType* type);

	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::BroType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::AddrType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::EnumType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::TypeList* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::OpaqueType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::RecordType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::SubNetType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::TableType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::TypeType* type);
	std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val, const ::VectorType* type);

};

}
}
}

#endif
