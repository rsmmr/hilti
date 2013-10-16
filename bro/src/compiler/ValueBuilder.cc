
#include <Val.h>
#undef List

#include <hilti/hilti.h>
#include <util/util.h>

#include "ValueBuilder.h"
#include "ModuleBuilder.h"

using namespace bro::hilti::compiler;

ValueBuilder::ValueBuilder(class ModuleBuilder* mbuilder)
	: BuilderBase(mbuilder)
	{
	}

shared_ptr<hilti::Expression> ValueBuilder::Compile(const ::Val* val)
	{
	switch ( val->Type()->Tag() ) {
	case TYPE_BOOL:
	case TYPE_COUNT:
	case TYPE_COUNTER:
	case TYPE_DOUBLE:
	case TYPE_FILE:
	case TYPE_FUNC:
	case TYPE_INT:
	case TYPE_INTERVAL:
	case TYPE_TIME:
	case TYPE_TYPE:
		return CompileBaseVal(static_cast<const ::Val*>(val));

	case TYPE_ADDR:
		return Compile(static_cast<const ::AddrVal*>(val));

	case TYPE_ENUM:
		return Compile(static_cast<const ::EnumVal*>(val));

	case TYPE_LIST:
		return Compile(static_cast<const ::ListVal*>(val));

	case TYPE_OPAQUE:
		return Compile(static_cast<const ::OpaqueVal*>(val));

	case TYPE_PATTERN:
		return Compile(static_cast<const ::PatternVal*>(val));

	case TYPE_PORT:
		return Compile(static_cast<const ::PortVal*>(val));

	case TYPE_RECORD:
		return Compile(static_cast<const ::RecordVal*>(val));

	case TYPE_STRING:
		return Compile(static_cast<const ::StringVal*>(val));

	case TYPE_SUBNET:
		return Compile(static_cast<const ::SubNetVal*>(val));

	case TYPE_TABLE:
		return Compile(static_cast<const ::TableVal*>(val));

	case TYPE_VECTOR:
		return Compile(static_cast<const ::VectorVal*>(val));

	default:
		Error(::util::fmt("unsupported value of type %s", ::type_name(val->Type()->Tag())));
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::CompileBaseVal(const ::Val* val)
	{
	switch ( val->Type()->Tag() ) {
	case TYPE_BOOL:
		return ::hilti::builder::boolean::create(val->AsBool());

	case TYPE_COUNT:
		{
		Error("no support yet for compiling Val of type TYPE_COUNT", val);
		return nullptr;
		}

	case TYPE_COUNTER:
		{
		Error("no support yet for compiling Val of type TYPE_COUNTER", val);
		return nullptr;
		}

	case TYPE_DOUBLE:
		{
		Error("no support yet for compiling Val of type TYPE_DOUBLE", val);
		return nullptr;
		}

	case TYPE_FILE:
		{
		Error("no support yet for compiling Val of type TYPE_FUNC", val);
		return nullptr;
		}

	case TYPE_FUNC:
		{
		auto func = val->AsFunc();
		ModuleBuilder()->DeclareFunction(func);
		return ::hilti::builder::id::create(HiltiSymbol(func));
		}

	case TYPE_INT:
		{
		Error("no support yet for compiling Val of type TYPE_INT", val);
		return nullptr;
		}

	case TYPE_INTERVAL:
		{
		Error("no support yet for compiling Val of type TYPE_INTERVAL", val);
		return nullptr;
		}

	case TYPE_TIME:
		{
		Error("no support yet for compiling Val of type TYPE_TIME", val);
		return nullptr;
		}

	case TYPE_TYPE:
		{
		Error("no support yet for compiling Val of type TYPE_TYPE", val);
		return nullptr;
		}

	default:
		Error("ValueBuilder: cannot be reached", val);
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::AddrVal* val)
	{
	Error("no support yet for compiling AddrVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::EnumVal* val)
	{
	Error("no support yet for compiling EnumVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::ListVal* val)
	{
	Error("no support yet for compiling ListVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::OpaqueVal* val)
	{
	Error("no support yet for compiling OpaqueVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::PatternVal* val)
	{
	Error("no support yet for compiling PatternVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::PortVal* val)
	{
	Error("no support yet for compiling PortVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::RecordVal* val)
	{
	Error("no support yet for compiling RecordVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::StringVal* val)
	{
	auto s = val->AsString();
	auto b = std::string((const char*)s->Bytes(), s->Len());
	return ::hilti::builder::bytes::create(b);
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::SubNetVal* val)
	{
	Error("no support yet for compiling SubNetVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::TableVal* val)
	{
	Error("no support yet for compiling TableVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::VectorVal* val)
	{
	Error("no support yet for compiling VectorVal", val);
	return nullptr;
	}
