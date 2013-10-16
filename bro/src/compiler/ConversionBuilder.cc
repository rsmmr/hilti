
#include <Type.h>
#undef List

#include <hilti/hilti.h>
#include <util/util.h>

#include "ConversionBuilder.h"

using namespace bro::hilti::compiler;

ConversionBuilder::ConversionBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

shared_ptr<::hilti::Expression> ConversionBuilder::ConvertBroToHilti(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	switch ( type->Tag() ) {
	case TYPE_ADDR:
	case TYPE_BOOL:
	case TYPE_COUNT:
	case TYPE_COUNTER:
	case TYPE_DOUBLE:
	case TYPE_FILE:
	case TYPE_FUNC:
	case TYPE_INT:
	case TYPE_INTERVAL:
	case TYPE_PATTERN:
	case TYPE_PORT:
	case TYPE_STRING:
	case TYPE_TIME:
	case TYPE_TYPE:
		return BroToHilti(val, static_cast<const ::BroType*>(type));

	case TYPE_ENUM:
		return BroToHilti(val, static_cast<const ::EnumType*>(type));

	case TYPE_LIST:
		return BroToHilti(val, static_cast<const ::TypeList*>(type));

	case TYPE_OPAQUE:
		return BroToHilti(val, static_cast<const ::OpaqueType*>(type));

	case TYPE_RECORD:
		return BroToHilti(val, static_cast<const ::RecordType*>(type));

	case TYPE_SUBNET:
		return BroToHilti(val, static_cast<const ::SubNetType*>(type));

	case TYPE_TABLE:
		return BroToHilti(val, static_cast<const ::TableType*>(type));

	case TYPE_VECTOR:
		return BroToHilti(val, static_cast<const ::VectorType*>(type));

	default:
		Error(::util::fmt("ConversionBuilder/B2H: unsupported value of type %s", ::type_name(type->Tag())));
	}

	// Cannot be reached.
	return nullptr;
	}

shared_ptr<::hilti::Expression> ConversionBuilder::ConvertHiltiToBro(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	switch ( type->Tag() ) {
	case TYPE_ADDR:
	case TYPE_BOOL:
	case TYPE_COUNT:
	case TYPE_COUNTER:
	case TYPE_DOUBLE:
	case TYPE_FILE:
	case TYPE_FUNC:
	case TYPE_INT:
	case TYPE_INTERVAL:
	case TYPE_PATTERN:
	case TYPE_PORT:
	case TYPE_STRING:
	case TYPE_TIME:
	case TYPE_TYPE:
		return HiltiToBro(val, static_cast<const ::BroType*>(type));

	case TYPE_ENUM:
		return HiltiToBro(val, static_cast<const ::EnumType*>(type));

	case TYPE_LIST:
		return HiltiToBro(val, static_cast<const ::TypeList*>(type));

	case TYPE_OPAQUE:
		return HiltiToBro(val, static_cast<const ::OpaqueType*>(type));

	case TYPE_RECORD:
		return HiltiToBro(val, static_cast<const ::RecordType*>(type));

	case TYPE_SUBNET:
		return HiltiToBro(val, static_cast<const ::SubNetType*>(type));

	case TYPE_TABLE:
		return HiltiToBro(val, static_cast<const ::TableType*>(type));

	case TYPE_VECTOR:
		return HiltiToBro(val, static_cast<const ::VectorType*>(type));

	default:
		Error(::util::fmt("ConversionBuilder/B2H: unsupported value of type %s", ::type_name(type->Tag())));
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");

	switch ( type->Tag() ) {
	case TYPE_ADDR:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_BOOL");
		return nullptr;
		}

	case TYPE_BOOL:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_BOOL");
		return nullptr;
		}

	case TYPE_COUNT:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_COUNT");
		return nullptr;
		}

	case TYPE_COUNTER:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_COUNTER");
		return nullptr;
		}

	case TYPE_DOUBLE:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_DOUBLE");
		return nullptr;
		}

	case TYPE_FILE:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_FUNC");
		return nullptr;
		}

	case TYPE_FUNC:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_FUNC");
		return nullptr;
		}

	case TYPE_INT:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_INT");
		return nullptr;
		}

	case TYPE_INTERVAL:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_INTERVAL");
		return nullptr;
		}

	case TYPE_PATTERN:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_PATTERN");
		return nullptr;
		}

	case TYPE_PORT:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_PORT");
		return nullptr;
		}

	case TYPE_STRING:
		{
		auto s = Builder()->addTmp("s", HiltiType(type));
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(s, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_string"), args);
		return s;
		}

	case TYPE_TIME:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_TIME");
		return nullptr;
		}

	case TYPE_TYPE:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_TYPE");
		return nullptr;
		}

	default:
		Error("ConversionBuilder/B2H: cannot be reached");
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::AddrType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting AddrVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::EnumType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting EnumVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::TypeList* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting ListVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::OpaqueType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting OpaqueVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::RecordType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting RecordVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::SubNetType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting SubNetVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::TableType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting TableVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::VectorType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting VectorVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");

	switch ( type->Tag() ) {
	case TYPE_ADDR:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_BOOL");
		return nullptr;
		}

	case TYPE_BOOL:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_BOOL");
		return nullptr;
		}

	case TYPE_COUNT:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_COUNT");
		return nullptr;
		}

	case TYPE_COUNTER:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_COUNTER");
		return nullptr;
		}

	case TYPE_DOUBLE:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_DOUBLE");
		return nullptr;
		}

	case TYPE_FILE:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_FUNC");
		return nullptr;
		}

	case TYPE_FUNC:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_FUNC");
		return nullptr;
		}

	case TYPE_INT:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_INT");
		return nullptr;
		}

	case TYPE_INTERVAL:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_INTERVAL");
		return nullptr;
		}

	case TYPE_PATTERN:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_PATTERN");
		return nullptr;
		}

	case TYPE_PORT:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_PORT");
		return nullptr;
		}

	case TYPE_STRING:
		{
		auto broval = Builder()->addTmp("val", vtype);
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(broval, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::h2b_bytes"), args);
		return broval;
		}

	case TYPE_TIME:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_TIME");
		return nullptr;
		}

	case TYPE_TYPE:
		{
		Error("ConversionBuilder/H2B: no support yet for converting Val of type TYPE_TYPE");
		return nullptr;
		}

	default:
		Error("ConversionBuilder/H2B: cannot be reached");
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::AddrType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting AddrVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::EnumType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting EnumVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::TypeList* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting ListVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::OpaqueType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting OpaqueVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::RecordType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting RecordVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::SubNetType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting SubNetVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::TableType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting TableVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::VectorType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting VectorVal");
	return nullptr;
	}
