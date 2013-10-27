
#include <Val.h>
#include <RE.h>
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
	case TYPE_COUNTER:
		return ::hilti::builder::integer::create(val->AsCount());

	case TYPE_DOUBLE:
		return ::hilti::builder::double_::create(val->AsDouble());

	case TYPE_FILE:
		{
		Error("no support yet for compiling Val of type TYPE_FUNC", val);
		return nullptr;
		}

	case TYPE_FUNC:
		{
		auto func = DeclareFunction(val->AsFunc());
		return func;
		}

	case TYPE_INT:
		return ::hilti::builder::integer::create(val->AsInt());

	case TYPE_INTERVAL:
		return ::hilti::builder::interval::create(val->AsInterval());

	case TYPE_TIME:
		return ::hilti::builder::time::create(val->AsTime());

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
	return ::hilti::builder::address::create(val->AsAddr().AsString());
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::EnumVal* val)
	{
	auto etype = HiltiType(val->Type());
	auto label = val->Type()->AsEnumType()->Lookup(val->AsEnum());
        auto id = ::hilti::builder::id::node(label);
	return ::hilti::builder::enum_::create(id, etype);
        }

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::ListVal* val)
	{
	::hilti::builder::tuple::element_list elems;

	for ( int i = 0; i < val->Length(); i++ )
		elems.push_back(HiltiValue(val->Index(i)));

	return ::hilti::builder::tuple::create(elems);
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::OpaqueVal* val)
	{
	Error("no support yet for compiling OpaqueVal", val);
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::PatternVal* val)
	{
	// TODO: We don't convert the regexp dialect yet.
	return ::hilti::builder::address::create(val->AsPattern()->PatternText());
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::PortVal* val)
	{
        ::hilti::constant::PortVal::Proto proto;

	switch ( val->PortType() ) {
	case ::TRANSPORT_TCP:
		proto = ::hilti::constant::PortVal::TCP;
		break;

	case ::TRANSPORT_UDP:
		proto = ::hilti::constant::PortVal::UDP;
		break;


	case ::TRANSPORT_ICMP:
		Error("no support yet for compiling PortVals of ICMP type", val);
		break;

	default:
		InternalError("unexpected port type in ValueBuilder::Compile");
	}

	return ::hilti::builder::port::create(val->Port(), proto);
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::RecordVal* val)
	{
	auto type = val->Type()->AsRecordType();

	::hilti::builder::struct_::element_list elems;

	for ( int i = 0; i < type->NumFields(); i++ )
		{
		auto f = val->Lookup(i);

		if ( f )
			elems.push_back(HiltiValue(f));
		else
			elems.push_back(::hilti::builder::unset::create());
		}

	return ::hilti::builder::struct_::create(elems);
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::StringVal* val)
	{
	auto s = val->AsString();
	auto b = std::string((const char*)s->Bytes(), s->Len());
	return ::hilti::builder::bytes::create(b);
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::SubNetVal* val)
	{
	return ::hilti::builder::network::create(val->AsSubNet().AsString());
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::TableVal* val)
	{
	if ( val->Type()->IsSet() )
		{
		::hilti::builder::set::element_list elems;

		auto tbl = val->AsTable();

		HashKey* h;
		TableEntryVal* v;

		IterCookie* c = tbl->InitForIteration();

		while ( (v = tbl->NextEntry(h, c)) )
			{
			shared_ptr<::hilti::Expression> hk;

			auto lk = val->RecoverIndex(h);
			if ( lk->Length() == 1 )
				hk = HiltiValue(lk->Index(0));
			else
				hk = HiltiValue(lk);

			elems.push_back(hk);

			delete h;
			}

		auto htype = HiltiType(val->Type());
		auto stype = ::ast::checkedCast<::hilti::type::Reference>(htype);
		return ::hilti::builder::set::create(true, stype->argType(), elems);
		}

	else
		{
		::hilti::builder::map::element_list elems;

		auto tbl = val->AsTable();

		HashKey* h;
		TableEntryVal* v;

		IterCookie* c = tbl->InitForIteration();

		while ( (v = tbl->NextEntry(h, c)) )
			{
			shared_ptr<::hilti::Expression> hk;

			auto lk = val->RecoverIndex(h);
			if ( lk->Length() == 1 )
				hk = HiltiValue(lk->Index(0));
			else
				hk = HiltiValue(lk);

			auto hv = HiltiValue(v->Value());
			auto e = ::hilti::builder::map::element(hk, hv);
			elems.push_back(e);

			delete h;
			}

		auto htype = HiltiType(val->Type());
		auto mtype = ::ast::checkedCast<::hilti::type::Reference>(htype);
		return ::hilti::builder::map::create(mtype->argType(), elems);
		}
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::VectorVal* val)
	{
	::hilti::builder::vector::element_list elems;

	for ( int i = 0; i < val->Size(); i++ )
		elems.push_back(HiltiValue(val->Lookup(i)));

	auto vt = HiltiType(val->Type());
	return ::hilti::builder::vector::create(vt, elems);
	}
