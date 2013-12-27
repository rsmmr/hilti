
#include <Val.h>
#include <RE.h>
#include <Func.h>
#include "ASTDumper.h"
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

shared_ptr<::hilti::Expression> ValueBuilder::DefaultInitValue(const ::BroType* type)
	{
	auto htype = HiltiType(type);
	auto rtype = ::ast::tryCast<::hilti::type::Reference>(htype);

	switch ( type->Tag() )	{
	case TYPE_TABLE:
		{
		if ( type->IsSet() )
			{
			auto stype = ::ast::checkedCast<::hilti::type::Set>(rtype->argType());
			return ::hilti::builder::set::create(stype->argType(), {});
			}

		else
			{
			auto mtype = ::ast::checkedCast<::hilti::type::Map>(rtype->argType());
			return ::hilti::builder::map::create(mtype->keyType(), mtype->valueType(), {});
			}
		}

	case TYPE_VECTOR:
		{
		auto vtype = ::ast::checkedCast<::hilti::type::Vector>(rtype->argType());
		return ::hilti::builder::vector::create(vtype->argType(), {});
		}

	case TYPE_RECORD:
		{
		return ::hilti::builder::struct_::create({});
		}

	case TYPE_STRING:
		return nullptr;

	case TYPE_FILE:
		return nullptr;

	default:
		// For non-reference types, HILTi's default should be right.
		if ( ! rtype )
			return nullptr;

	Error(::util::fmt("ValueBuilder does not define a initialization value for type %s", ::type_name(type->Tag())), type);
	}

	// Cannot be reached.
	assert(0);
	return 0;
	}

shared_ptr<::hilti::Expression> ValueBuilder::TypeVal(const ::BroType* type)
	{
	// TODO: Unify with ConversionBuilder. We shouldn't really
	// look up the type by name each time we need, but precompute
	// and sotre in a global.
	if ( ! type->GetTypeID() )
		{
		ODesc d;
		type->Describe(&d);
		Error(::util::fmt("ValueBuilder: type value without type id (%s)", d.Description()));
		}

	auto tmp = Builder()->addTmp("ttype", ::hilti::builder::type::byName("LibBro::BroType"));
	auto f = ::hilti::builder::id::create("LibBro::bro_lookup_type_as_val");
	auto args = ::hilti::builder::tuple::create( { ::hilti::builder::string::create(type->GetTypeID()) } );
	Builder()->addInstruction(tmp, ::hilti::instruction::flow::CallResult, f, args);
	return tmp;
	}

shared_ptr<::hilti::Expression> ValueBuilder::FunctionVal(const ::Func* func)
	{
	auto tmp = Builder()->addTmp("func", ::hilti::builder::type::byName("LibBro::BroVal"));
	auto f = ::hilti::builder::id::create("LibBro::bro_lookup_id_as_val");
	auto args = ::hilti::builder::tuple::create( { ::hilti::builder::string::create(func->Name()) } );
	Builder()->addInstruction(tmp, ::hilti::instruction::flow::CallResult, f, args);
	return tmp;
	}

shared_ptr<::hilti::Expression> ValueBuilder::BroType(const ::BroType* type)
	{
	// TODO: Unify with ConversionBuilder. We shouldn't really
	// look up the type by name each time we need, but precompute
	// and sotre in a global.
	if ( ! type->GetTypeID() )
		{
		ODesc d;
		type->Describe(&d);
		Error(::util::fmt("ValueBuilder: type value without type id (%s)", d.Description()));
		}

	auto tmp = Builder()->addLocal("ttype", ::hilti::builder::type::byName("LibBro::BroType"));
	auto f = ::hilti::builder::id::create("LibBro::bro_lookup_type");
	auto args = ::hilti::builder::tuple::create( { ::hilti::builder::string::create(type->GetTypeID()) } );
	Builder()->addInstruction(tmp, ::hilti::instruction::flow::CallResult, f, args);
	return tmp;
	}

shared_ptr<hilti::Expression> ValueBuilder::Compile(const ::Val* val, const ::BroType* target_type, bool init)
	{
	shared_ptr<::hilti::Expression> e = nullptr;

	target_types.push_back(target_type);
	is_init = init;

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
		e = CompileBaseVal(static_cast<const ::Val*>(val));
		break;

	case TYPE_ADDR:
		e = Compile(static_cast<const ::AddrVal*>(val));
		break;

	case TYPE_ENUM:
		e = Compile(static_cast<const ::EnumVal*>(val));
		break;

	case TYPE_LIST:
		e = Compile(static_cast<const ::ListVal*>(val));
		break;

	case TYPE_OPAQUE:
		e = Compile(static_cast<const ::OpaqueVal*>(val));
		break;

	case TYPE_PATTERN:
		e = Compile(static_cast<const ::PatternVal*>(val));
		break;

	case TYPE_PORT:
		e = Compile(static_cast<const ::PortVal*>(val));
		break;

	case TYPE_RECORD:
		e = Compile(static_cast<const ::RecordVal*>(val));
		break;

	case TYPE_STRING:
		e = Compile(static_cast<const ::StringVal*>(val));
		break;

	case TYPE_SUBNET:
		e = Compile(static_cast<const ::SubNetVal*>(val));
		break;

	case TYPE_TABLE:
		e = Compile(static_cast<const ::TableVal*>(val));
		break;

	case TYPE_VECTOR:
		e = Compile(static_cast<const ::VectorVal*>(val));
		break;

	default:
		Error(::util::fmt("unsupported value of type %s", ::type_name(val->Type()->Tag())));
	}

	target_types.pop_back();

	if ( target_type && target_type->Tag() == ::TYPE_ANY )
		// Need to pass an actual Bro value.
		e = RuntimeHiltiToVal(e, val->Type());

	return e;
	}

const ::BroType* ValueBuilder::TargetType() const
	{
	if ( ! target_types.size() || ! target_types.back() )
		{
		Error("ValueBuilder::TargetType() used, but no target type set.");
		return 0;
		}

	auto t = target_types.back();
	assert(t);
	return t;
	}

bool ValueBuilder::HasTargetType() const
	{
	return target_types.size() && target_types.back();
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
		Error("no support yet for compiling Val of type TYPE_FILE", val);
		return nullptr;
		}

	case TYPE_FUNC:
		DeclareFunction(val->AsFunc());
		return HiltiBroVal(val->AsFunc());

	case TYPE_INT:
		return ::hilti::builder::integer::create(val->AsInt());

	case TYPE_INTERVAL:
		return ::hilti::builder::interval::create(val->AsInterval());

	case TYPE_TIME:
		return ::hilti::builder::time::create(val->AsTime());

	case TYPE_TYPE:
		return HiltiBroType(val->Type());

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
	auto name = ::util::strreplace(label, "::", "_");
        auto id = ::hilti::builder::id::node(name);
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

	// The anywhere pattern has the form "^?(.|\n)*(<real-stuff-here>)"
	auto pt = std::string(val->AsPattern()->AnywherePatternText());
	auto ps = pt.substr(10, pt.size() - 10 - 1);
	auto p = ::hilti::builder::regexp::pattern(ps, "");
	return ::hilti::builder::regexp::create(p);
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
			{
			auto ftype = type->FieldType(i);
			elems.push_back(HiltiValue(f, ftype));
			}
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
	auto ttype = val->Type()->AsTableType();

	if ( ttype->IsUnspecifiedTable() )
		{
		auto tt = TargetType();
		auto rt = ast::checkedCast<::hilti::type::Reference>(HiltiType(tt));

		if ( tt->IsSet() )
			{
			auto stype = ast::checkedCast<::hilti::type::Set>(rt->argType());
			return ::hilti::builder::set::create(stype->argType(), {});
			}

		else
			{
			auto mtype = ast::checkedCast<::hilti::type::Map>(rt->argType());
			return ::hilti::builder::map::create(mtype->keyType(), mtype->valueType(), {});
			}
		}

	if ( ttype->IsSet() )
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
		auto rtype = ::ast::checkedCast<::hilti::type::Reference>(htype);
		auto stype = ::ast::checkedCast<::hilti::type::Set>(rtype->argType());
		return ::hilti::builder::set::create(stype->argType(), elems);
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
		auto rtype = ::ast::checkedCast<::hilti::type::Reference>(htype);
		auto mtype = ::ast::checkedCast<::hilti::type::Map>(rtype->argType());


		shared_ptr<::hilti::Expression> def;

		if ( Attr* def_attr = val->FindAttr(ATTR_DEFAULT) )
			{
			const ::Expr* expr = def_attr->AttrExpr();
			const ::BroType* ytype = ttype->YieldType();
			const ::BroType* dtype = expr->Type();

			if ( dtype->Tag() == TYPE_RECORD && ytype->Tag() == TYPE_RECORD &&
			     ! ::same_type(dtype, ytype) )
				Error("no support yet for record_coercion in table &default", val);

			if ( dtype->Tag() != TYPE_FUNC )
				def = HiltiExpression(expr, ytype);
			else
				{
				auto m = BroExprToFunc(expr);

				if ( m.first && m.second )
					{
					auto hilti_func = DeclareFunction(m.second);
					def = ::hilti::builder::callable::create(mtype->valueType(),
										 { mtype->keyType() },
										 hilti_func,
										 {});
					}

				else
					Error("no support for non-const table default functions");
				}
			}

		return ::hilti::builder::map::create(mtype->keyType(), mtype->valueType(), elems, def);
		}
	}

std::shared_ptr<::hilti::Expression> ValueBuilder::Compile(const ::VectorVal* val)
	{
	auto vtype = val->Type()->AsVectorType();
	auto ytype = val->Type()->AsVectorType()->YieldType();

	if ( vtype->IsUnspecifiedVector() )
		{
		auto tt = TargetType();
		auto rt = ast::checkedCast<::hilti::type::Reference>(HiltiType(tt));
		auto vtype = ast::checkedCast<::hilti::type::Vector>(rt->argType());
		return ::hilti::builder::vector::create(vtype->argType(), {});
		}

	::hilti::builder::vector::element_list elems;

	for ( int i = 0; i < val->Size(); i++ )
		elems.push_back(HiltiValue(val->Lookup(i), ytype));

	return ::hilti::builder::vector::create(HiltiType(ytype), elems);
	}
