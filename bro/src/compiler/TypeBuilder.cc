
#include <Type.h>
#undef List

#include <hilti/builder/nodes.h>
#include <util/util.h>

#include "TypeBuilder.h"

using namespace bro::hilti::compiler;

TypeBuilder::TypeBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

shared_ptr<hilti::Type> TypeBuilder::Compile(const ::BroType* type)
	{
	switch ( type->Tag() ) {
	case TYPE_ADDR:
	case TYPE_BOOL:
	case TYPE_COUNT:
	case TYPE_COUNTER:
	case TYPE_DOUBLE:
	case TYPE_INT:
	case TYPE_INTERVAL:
	case TYPE_PATTERN:
	case TYPE_PORT:
	case TYPE_STRING:
	case TYPE_TIME:
		return CompileBaseType(static_cast<const ::BroType*>(type));

	case TYPE_ENUM:
		return Compile(static_cast<const ::EnumType*>(type));

	case TYPE_FILE:
		return Compile(static_cast<const ::FileType*>(type));

	case TYPE_FUNC:
		return Compile(static_cast<const ::FuncType*>(type));

	case TYPE_OPAQUE:
		return Compile(static_cast<const ::OpaqueType*>(type));

	case TYPE_RECORD:
		return Compile(static_cast<const ::RecordType*>(type));

	case TYPE_SUBNET:
		return Compile(static_cast<const ::SubNetType*>(type));

	case TYPE_TABLE:
		return Compile(static_cast<const ::TableType*>(type));

	case TYPE_LIST:
		return Compile(static_cast<const ::TypeList*>(type));

	case TYPE_TYPE:
		return Compile(static_cast<const ::TypeType*>(type));

	case TYPE_VECTOR:
		return Compile(static_cast<const ::VectorType*>(type));

	default:
		Error(::util::fmt("unsupported type %s", ::type_name(type->Tag())));
	}

	// Cannot be reached.
	return nullptr;
	}


std::shared_ptr<::hilti::Type> TypeBuilder::CompileBaseType(const ::BroType* type)
	{
	switch ( type->Tag() ) {
	case TYPE_ADDR:
		{
		Error("no support yet for compiling base type TYPE_ADDR", type);
		return nullptr;
		}

	case TYPE_BOOL:
		return ::hilti::builder::boolean::type();

	case TYPE_COUNT:
		{
		Error("no support yet for compiling base type TYPE_COUNT", type);
		return nullptr;
		}

	case TYPE_COUNTER:
		{
		Error("no support yet for compiling base type TYPE_COUNTER", type);
		return nullptr;
		}

	case TYPE_DOUBLE:
		{
		Error("no support yet for compiling base type TYPE_DOUBLE", type);
		return nullptr;
		}

	case TYPE_INT:
		{
		Error("no support yet for compiling base type TYPE_INT", type);
		return nullptr;
		}

	case TYPE_INTERVAL:
		{
		Error("no support yet for compiling base type TYPE_INTERVAL", type);
		return nullptr;
		}

	case TYPE_PATTERN:
		{
		Error("no support yet for compiling base type TYPE_PATTERN", type);
		return nullptr;
		}

	case TYPE_PORT:
		{
		Error("no support yet for compiling base type TYPE_PORT", type);
		return nullptr;
		}

	case TYPE_STRING:
		return ::hilti::builder::reference::type(::hilti::builder::bytes::type());

	case TYPE_TIME:
		{
		Error("no support yet for compiling base type TYPE_TIME", type);
		return nullptr;
		}

	default:
		Error("TypeBuilder: cannot be reached", type);
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::EnumType* type)
	{
	Error("no support yet for compiling EnumType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::FileType* type)
	{
	Error("no support yet for compiling FileType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::FuncType* type)
	{
	auto byield = type->YieldType();
	auto bargs = type->Args();

	std::shared_ptr<::hilti::Type> hyield;

	if ( byield && byield->Tag() != TYPE_VOID && byield->Tag() != TYPE_ANY )
		hyield = HiltiType(byield);
	else
		hyield = ::hilti::builder::void_::type();

	auto hresult = ::hilti::builder::function::result(hyield, false);
	auto hargs = ::hilti::builder::function::parameter_list();

	for ( int i = 0; i < bargs->NumFields(); i++ )
		{
		auto name = bargs->FieldName(i);
		auto type = HiltiType(bargs->FieldType(i));
		auto def = bargs->FieldDefault(i) ? HiltiValue(bargs->FieldDefault(i)) : nullptr;

		auto param = ::hilti::builder::function::parameter(name, type, false, def);
		hargs.push_back(param);
		}

	switch ( type->Flavor() ) {
	case FUNC_FLAVOR_FUNCTION:
		return ::hilti::builder::function::type(hresult, hargs);

	case FUNC_FLAVOR_EVENT:
	case FUNC_FLAVOR_HOOK:
		return ::hilti::builder::hook::type(hresult, hargs);

	default:
		InternalError("unknown function flavor", type);
	}

	InternalError("cannot be reached", type);
	}


std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::OpaqueType* type)
	{
	Error("no support yet for compiling OpaqueType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::RecordType* type)
	{
	Error("no support yet for compiling RecordType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::SubNetType* type)
	{
	Error("no support yet for compiling SubNetType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::TableType* type)
	{
	Error("no support yet for compiling TableType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::TypeList* type)
	{
	Error("no support yet for compiling TypeList", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::TypeType* type)
	{
	Error("no support yet for compiling TypeType", type);
	return nullptr;
	}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::VectorType* type)
	{
	Error("no support yet for compiling VectorType", type);
	return nullptr;
	}
