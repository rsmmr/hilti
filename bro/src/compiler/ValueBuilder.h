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
	 * @return The converted expression.
	 */
	shared_ptr<::hilti::Expression> Compile(const ::Val* val);

protected:
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
};

}
}
}

#endif
