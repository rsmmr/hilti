
#include <Type.h>
#undef List

#include <hilti/hilti.h>
#include <util/util.h>

#include "ConversionBuilder.h"
#include "ModuleBuilder.h"
#include "Compiler.h"

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

	auto dst = Builder()->addTmp("b2h", HiltiType(type));

	switch ( type->Tag() ) {
	case TYPE_ADDR:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_address"), args);
		return dst;
		}

	case TYPE_BOOL:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_bool"), args);
		return dst;
		}

	case TYPE_COUNT:
	case TYPE_COUNTER:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_count"), args);
		return dst;
		}

	case TYPE_DOUBLE:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_double"), args);
		return dst;
		}

	case TYPE_FILE:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_FILE");
		return nullptr;
		}

	case TYPE_FUNC:
		{
		Error("ConversionBuilder/B2H: no support yet for converting Val of type TYPE_FUNC");
		return nullptr;
		}

	case TYPE_INT:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_integer"), args);
		return dst;
		}

	case TYPE_INTERVAL:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_interval"), args);
		return dst;
		}

	case TYPE_PATTERN:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_pattern"), args);
		return dst;
		}

	case TYPE_PORT:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_port"), args);
		return dst;
		}

	case TYPE_STRING:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_string"), args);
		return dst;
		}

	case TYPE_TIME:
		{
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::b2h_time"), args);
		return dst;
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
	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");
	auto dst = Builder()->addTmp("b2h", HiltiType(type));

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::b2h_subnet"), args);

	return dst;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BuildConversionFunction(const char* tag,
										shared_ptr<::hilti::Expression> val,
										shared_ptr<::hilti::Type> valtype,
										shared_ptr<::hilti::Type> dsttype,
										const ::BroType* type,
										build_conversion_function_callback cb)
	{
	auto mbuilder = ModuleBuilder();
	auto glue_mbuilder = GlueModuleBuilder();
	auto glue_ns = glue_mbuilder->module()->id()->name();

	auto fname = ::util::fmt("%s_%s", tag, HiltiODescSymbol(type));
	auto qualified_fname = ::util::fmt("%s::%s_%s", glue_ns, tag, HiltiODescSymbol(type));

	auto result = ::hilti::builder::function::result(dsttype);
	auto param = ::hilti::builder::function::parameter("val", valtype, false, nullptr);

	if ( ! glue_mbuilder->declared(fname) )
		{
		Compiler()->pushModuleBuilder(glue_mbuilder);
		auto func = glue_mbuilder->pushFunction(fname, result, { param });
		glue_mbuilder->exportID(func->id());

		auto dst = Builder()->addTmp("dst", dsttype);
		cb(dst, ::hilti::builder::id::create("val"), type);
		Builder()->addInstruction(::hilti::instruction::flow::ReturnResult, dst);

		glue_mbuilder->popFunction();

		Compiler()->popModuleBuilder();
		}

	if ( ! mbuilder->declared(qualified_fname) )
		mbuilder->declareFunction(qualified_fname, result, { param });

	auto tmp = Builder()->addTmp("conv", dsttype);

	Builder()->addInstruction(tmp,
				  ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create(qualified_fname),
				  ::hilti::builder::tuple::create({ val }));

	return tmp;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BuildCreateBroType(const char* tag,
									   const ::BroType* type,
									   build_create_type_callback cb)
	{
	auto btype = ::hilti::builder::type::byName("LibBro::BroType");

	auto mbuilder = ModuleBuilder();
	auto glue_mbuilder = GlueModuleBuilder();
	auto glue_ns = glue_mbuilder->module()->id()->name();

	auto tname = ::util::fmt("%s_%s", tag, HiltiODescSymbol(type));
	auto qualified_tname = ::util::fmt("%s::%s_%s", glue_ns, tag, HiltiODescSymbol(type));

	if ( ! glue_mbuilder->declared(tname) )
		{
		auto global = glue_mbuilder->addGlobal(tname, btype);
		glue_mbuilder->exportID(tname);

		Compiler()->pushModuleBuilder(glue_mbuilder);
		glue_mbuilder->pushModuleInit();

		cb(global, type);

		glue_mbuilder->popFunction();
		Compiler()->popModuleBuilder();
		}

	if ( ! mbuilder->declared(qualified_tname) )
		mbuilder->declareGlobal(qualified_tname, btype);

	return mbuilder != glue_mbuilder ? ::hilti::builder::id::create(qualified_tname)
					 : ::hilti::builder::id::create(tname);
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::TableType* type)
	{
	// TODO: Check that we get Bro's reference counting right.

	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");

	return BuildConversionFunction("b2h", val, vtype, HiltiType(type), type,
		[&] (shared_ptr<::hilti::Expression> dst, shared_ptr<::hilti::Expression> val, const ::BroType* type)
		{
		auto mbuilder = ModuleBuilder();

		auto rtype = ast::checkedCast<type::Reference>(HiltiType(type));
		auto mtype = ast::checkedCast<type::Map>(rtype->argType());
		auto expr_mtype = ::hilti::builder::type::create(mtype);
		auto expr_ktype = ::hilti::builder::type::create(mtype->keyType());
		auto expr_vtype = ::hilti::builder::type::create(mtype->valueType());

		Builder()->addInstruction(dst, ::hilti::instruction::map::New, expr_mtype);

		auto loop = mbuilder->pushBuilder("loop");

		auto cookie = Builder()->addTmp("cookie", ::hilti::builder::caddr::type());
		auto kval = Builder()->addTmp("k", vtype);
		auto vval = Builder()->addTmp("v", vtype);

		auto ttype = ::hilti::builder::tuple::type({ ::hilti::builder::caddr::type(), vtype, vtype });
		auto t = Builder()->addTmp("t", ttype);

		Builder()->addInstruction(t,
					::hilti::instruction::flow::CallResult,
					::hilti::builder::id::create("LibBro::bro_table_iterate"),
					::hilti::builder::tuple::create({ val, cookie }));

		Builder()->addInstruction(cookie, ::hilti::instruction::tuple::Index, t, ::hilti::builder::integer::create(0));
		Builder()->addInstruction(kval, ::hilti::instruction::tuple::Index, t, ::hilti::builder::integer::create(1));
		Builder()->addInstruction(vval, ::hilti::instruction::tuple::Index, t, ::hilti::builder::integer::create(2));

		auto blocks = Builder()->addIf(kval);
		auto cont = std::get<0>(blocks);
		auto done = std::get<1>(blocks);

		mbuilder->popBuilder(loop);

		mbuilder->pushBuilder(cont);

		auto itypes = type->AsTableType()->Indices();
		const ::BroType* index_type = itypes;

		if ( itypes->Types()->length() == 1 )
			index_type = (*itypes->Types())[0];

		auto k = RuntimeValToHilti(kval, index_type);
		auto v = RuntimeValToHilti(vval, type->AsTableType()->YieldType());

		Builder()->addInstruction(::hilti::instruction::map::Insert, dst, k, v);
		Builder()->addInstruction(::hilti::instruction::flow::Jump, loop->block());

		mbuilder->popBuilder(cont);

		mbuilder->pushBuilder(done);
		});
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::BroToHilti(shared_ptr<::hilti::Expression> val, const ::VectorType* type)
	{
	Error("ConversionBuilder/B2H: no support yet for converting VectorVal");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::BroType* type)
	{
	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");
	auto dst = Builder()->addTmp("val", vtype);

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
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::h2b_count"), args);
		return dst;
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
		auto args = ::hilti::builder::tuple::create( { val } );
		Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::h2b_bytes"), args);
		return dst;
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
	// TODO: Check that we get Bro's reference counting right.
	//
	auto vtype = ::hilti::builder::type::byName("LibBro::BroVal");

	return BuildConversionFunction("h2b", val, HiltiType(type), vtype, type,
		[&] (shared_ptr<::hilti::Expression> dst, shared_ptr<::hilti::Expression> val, const ::BroType* type)
		{
		auto mbuilder = ModuleBuilder();
		auto rtype = ast::checkedCast<type::Reference>(HiltiType(type));
		auto mtype = ast::checkedCast<type::Map>(rtype->argType());

		auto cur = mbuilder->addTmp("cur", mtype->iterType());
		auto end = mbuilder->addTmp("end", mtype->iterType());
		auto t = mbuilder->addTmp("t", mtype->elementType());
		auto is_end = mbuilder->addTmp("is_end", ::hilti::builder::boolean::type());

		Builder()->addInstruction(dst,
					  ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::bro_table_new"),
					  ::hilti::builder::tuple::create({ CreateBroType(type) }));

		Builder()->addInstruction(cur, ::hilti::instruction::operator_::Begin, val);
		Builder()->addInstruction(end, ::hilti::instruction::operator_::End, val);

		auto loop = mbuilder->pushBuilder("loop");

		Builder()->addInstruction(is_end, ::hilti::instruction::operator_::Equal, cur, end);

		auto blocks = Builder()->addIf(is_end);
		auto done = std::get<0>(blocks);
		auto cont = std::get<1>(blocks);

		mbuilder->popBuilder(loop);

		mbuilder->pushBuilder(cont);

		Builder()->addInstruction(t, ::hilti::instruction::operator_::Deref, cur);

		auto k = mbuilder->addTmp("k", mtype->keyType());
		auto v = mbuilder->addTmp("v", mtype->valueType());

		Builder()->addInstruction(k, ::hilti::instruction::tuple::Index, t, ::hilti::builder::integer::create(0));
		Builder()->addInstruction(v, ::hilti::instruction::tuple::Index, t, ::hilti::builder::integer::create(1));

		auto itypes = type->AsTableType()->Indices();
		const ::BroType* index_type = itypes;

		if ( itypes->Types()->length() == 1 )
			index_type = (*itypes->Types())[0];

		auto kval = RuntimeHiltiToVal(k, index_type);
		auto vval = RuntimeHiltiToVal(v, type->AsTableType()->YieldType());

		Builder()->addInstruction(::hilti::instruction::flow::CallVoid,
					  ::hilti::builder::id::create("LibBro::bro_table_insert"),
					  ::hilti::builder::tuple::create({ dst, kval, vval }));

		Builder()->addInstruction(cur, ::hilti::instruction::operator_::Incr, cur);

		Builder()->addInstruction(::hilti::instruction::flow::Jump, loop->block());

		mbuilder->popBuilder(cont);

		mbuilder->pushBuilder(done);
		});
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::HiltiToBro(shared_ptr<::hilti::Expression> val, const ::VectorType* type)
	{
	Error("ConversionBuilder/H2B: no support yet for converting VectorVal");
	return nullptr;
	}

shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::BroType* type)
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
		return CreateBroTypeBase(static_cast<const ::BroType*>(type));

	case TYPE_ENUM:
		return CreateBroType(static_cast<const ::EnumType*>(type));

	case TYPE_LIST:
		return CreateBroType(static_cast<const ::TypeList*>(type));

	case TYPE_OPAQUE:
		return CreateBroType(static_cast<const ::OpaqueType*>(type));

	case TYPE_RECORD:
		return CreateBroType(static_cast<const ::RecordType*>(type));

	case TYPE_SUBNET:
		return CreateBroType(static_cast<const ::SubNetType*>(type));

	case TYPE_TABLE:
		return CreateBroType(static_cast<const ::TableType*>(type));

	case TYPE_VECTOR:
		return CreateBroType(static_cast<const ::VectorType*>(type));

	default:
		Error(::util::fmt("ConversionBuilder/CBT: unsupported value of type %s", ::type_name(type->Tag())));
	}

	// Cannot be reached.
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroTypeBase(const ::BroType* type)
	{
	auto dst = Builder()->addTmp("btype", ::hilti::builder::type::byName("LibBro::BroType"));
	auto tag = ::hilti::builder::integer::create(type->Tag());

	Builder()->addInstruction(dst,
				  ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::bro_base_type"),
				  ::hilti::builder::tuple::create({ tag }));

	return dst;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::EnumType* type)
	{
	Error("ConversionBuilder/CBT: no support yet for converting EnumType");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::TypeList* type)
	{
	return BuildCreateBroType("list", type,
		[&] (shared_ptr<::hilti::Expression> dst, const ::BroType* type)
		{
		auto ltype = type->AsTypeList();
		auto pure_type = ltype->IsPure() ? CreateBroType(ltype->PureType()) : ::hilti::builder::caddr::create();

		Builder()->addInstruction(dst,
					  ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::bro_list_type_new"),
					  ::hilti::builder::tuple::create({ pure_type }));

		auto types = ltype->Types();

		loop_over_list(*types, i)
			{
			auto t = CreateBroType((*types)[i]);
			Builder()->addInstruction(::hilti::instruction::flow::CallVoid,
						  ::hilti::builder::id::create("LibBro::bro_list_type_append"),
						  ::hilti::builder::tuple::create({ dst, t }));
			}
		});
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::OpaqueType* type)
	{
	Error("ConversionBuilder/CBT: no support yet for converting OpaqueType");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::RecordType* type)
	{
	Error("ConversionBuilder/CBT: no support yet for converting RecordType");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::SubNetType* type)
	{
	Error("ConversionBuilder/CBT: no support yet for converting SubNetType");
	return nullptr;
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::TableType* type)
	{
	return BuildCreateBroType("table", type,
		[&] (shared_ptr<::hilti::Expression> dst, const ::BroType* type)
		{
		auto k = CreateBroType(type->AsTableType()->Indices());
		auto v = CreateBroType(type->AsTableType()->YieldType());

		Builder()->addInstruction(dst,
					  ::hilti::instruction::flow::CallResult,
					  ::hilti::builder::id::create("LibBro::bro_table_type_new"),
					  ::hilti::builder::tuple::create({ k, v }));
		});
	}

std::shared_ptr<::hilti::Expression> ConversionBuilder::CreateBroType(const ::VectorType* type)
	{
	Error("ConversionBuilder/CBT: no support yet for converting VectorType");
	return nullptr;
	}
