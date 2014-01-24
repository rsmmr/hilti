
#include "Obj.h"
#include "Type.h"
#include "Plugin.h"
#undef List

#include <hilti/builder/builder.h>
#include <binpac/type.h>

#include "Converter.h"
#include "RuntimeInterface.h"
#include "LocalReporter.h"
#include "compiler/ConversionBuilder.h"
#include "compiler/ModuleBuilder.h"

using namespace bro::hilti;

TypeConverter::TypeConverter()
	{
	}

BroType* TypeConverter::Convert(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::binpac::Type> btype)
	{
	setArg1(btype);
	BroType* rtype;
	processOne(type, &rtype);
	return rtype;
	}

string TypeConverter::CacheIndex(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::binpac::Type> btype)
	{
	return ::util::fmt("%p-%p", type, type);
        }

BroType* TypeConverter::LookupCachedType(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::binpac::Type> btype)
	{
        auto i = type_cache.find(CacheIndex(type, btype));
	return i != type_cache.end() ? i->second.first : nullptr;
        }

uint64_t TypeConverter::TypeIndex(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::binpac::Type> btype)
	{
	Convert(type, btype); // Make sure we have the type cached.
	auto i = type_cache.find(CacheIndex(type, btype));
	assert(i != type_cache.end());
	return i->second.second;
        }

void TypeConverter::CacheType(std::shared_ptr<::hilti::Type> type, std::shared_ptr<::binpac::Type> btype, BroType* bro_type)
	{
	auto cache_idx = CacheIndex(type, btype);
	auto type_idx = lib_bro_add_indexed_type(bro_type);
	type_cache.insert(std::make_pair(cache_idx, std::make_pair(bro_type, type_idx)));
	}

ValueConverter::ValueConverter(compiler::ModuleBuilder* arg_mbuilder,
			       TypeConverter* arg_type_converter)
	{
	mbuilder = arg_mbuilder;
	type_converter = arg_type_converter;
	}

bool ValueConverter::Convert(shared_ptr<::hilti::Expression> value, shared_ptr<::hilti::Expression> dst, std::shared_ptr<::binpac::Type> btype, BroType* hint)
	{
	_bro_type_hints.push_back(hint);
	setArg1(value);
	setArg2(dst);
	_arg3 = btype;
	bool set = false;
	bool success = processOne(value->type(), &set);
	assert(set);
	_arg3 = nullptr;
	_bro_type_hints.pop_back();
	return success;
	}

shared_ptr<::binpac::Type> ValueConverter::arg3() const
	{
	return _arg3;
	}

shared_ptr<::hilti::builder::BlockBuilder> ValueConverter::Builder() const
	{
	return mbuilder->builder();
	}

void TypeConverter::visit(::hilti::type::Reference* b)
	{
	BroType* rtype;
	processOne(b->argType(), &rtype);
	setResult(rtype);
	}

void TypeConverter::visit(::hilti::type::Address* a)
	{
	auto result = base_type(TYPE_ADDR);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Bool* a)
	{
	auto result = base_type(TYPE_BOOL);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Bytes* b)
	{
	auto result = base_type(TYPE_STRING);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Double* d)
	{
	auto result = base_type(TYPE_DOUBLE);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Enum* e)
	{
	auto etype = ast::checkedCast<binpac::type::Enum>(arg1());

	if ( auto result = LookupCachedType(e->sharedPtr<::hilti::type::Enum>(), etype) )
		{
		setResult(result);
		return;
		}

	if ( ! etype->id() )
		reporter::internal_error("TypeConverter::visit(Enum): type has not ID");

	string module;
	string name;
	auto c = etype->id()->path();

	if ( c.size() == 1 )
		{
		name = c.front();

		if ( auto m = etype->id()->firstParent<binpac::Module>() )
			module = m->id()->pathAsString();
		}

	else if ( c.size() == 2 )
		{
		module = c.front();
		name = c.back();
		}

	if ( module.empty() || name.empty() )
                reporter::internal_error(::util::fmt("TypeConverter::visit(Enum): cannot determine type's module/name (%s/%s|%s)",
						     module.size(), name.size(), etype->id()->pathAsString().c_str()));

	// Build the Bro type.
	auto type_id = etype->id()->name();
	auto eresult = new EnumType(name);
	auto undef = ::util::fmt("%s_Undef", type_id);
	eresult->AddName(module, undef.c_str(), lib_bro_enum_undef_val, true);

	for ( auto l : etype->labels() )
		{
		if ( l.first->name() == "Undef" )
			continue;

		auto name = ::util::fmt("%s_%s", type_id, l.first->name());
		eresult->AddName(module, name.c_str(), l.second, true);
		}

	PLUGIN_DBG_LOG(HiltiPlugin, "Adding Bro enum type %s::%s", module.c_str(), name.c_str());
	string id_name = ::util::fmt("%s::%s", module, name);
	::ID* id = install_ID(name.c_str(), module.c_str(), true, true);
	id->SetType(eresult);
	id->MakeType();

	CacheType(e->sharedPtr<::hilti::type::Enum>(), etype, eresult);
	setResult(eresult);
	}

void TypeConverter::visit(::hilti::type::Integer* i)
	{
	auto itype = ast::checkedCast<binpac::type::Integer>(arg1());

	auto result = itype->signed_() ? base_type(TYPE_INT) : base_type(TYPE_COUNT);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::String* s)
	{
	auto result = base_type(TYPE_STRING);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Time* t)
	{
	auto result = base_type(TYPE_TIME);
	setResult(result);
    }

void TypeConverter::visit(::hilti::type::Interval* t)
	{
	auto result = base_type(TYPE_INTERVAL);
	setResult(result);
    }

void TypeConverter::visit(::hilti::type::Tuple* t)
	{
	auto btype = ast::checkedCast<binpac::type::Tuple>(arg1());

	auto tdecls = new ::type_decl_list;

	auto i = 0;

	for ( auto m : ::util::zip2(t->typeList(), btype->typeList()) )
		{
		auto name = ::util::fmt("f%d", i++);
		auto td = new ::TypeDecl(Convert(m.first, m.second), copy_string(name.c_str()), 0, true);
		tdecls->append(td);
		}

	auto result = new ::RecordType(tdecls);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::List* t)
	{
	auto btype = ast::checkedCast<binpac::type::List>(arg1());
	auto itype = Convert(t->argType(), btype->argType());
	auto result = new ::VectorType(itype);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Vector* t)
	{
	auto btype = ast::checkedCast<binpac::type::Vector>(arg1());
	auto itype = Convert(t->argType(), btype->argType());
	auto result = new ::VectorType(itype);
	setResult(result);
	}

void TypeConverter::visit(::hilti::type::Set* t)
	{
	auto btype = ast::checkedCast<binpac::type::Set>(arg1());
	auto itype = Convert(t->argType(), btype->argType());

	auto types = new ::TypeList();
	types->Append(itype); // FIXME: No tuple indices yet.
	auto result = new ::TableType(types, 0);
	setResult(result);
	}

void ValueConverter::visit(::hilti::type::Reference* b)
	{
	bool set = false;
	processOne(b->argType(), &set);
	assert(set);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Integer* i)
	{
	auto val = arg1();
	auto dst = arg2();
	auto itype = ast::checkedCast<binpac::type::Integer>(arg3());

	const char* func = "";
	shared_ptr<::hilti::Instruction> ext = 0;

	if ( itype->signed_() )
		{
		func = "LibBro::h2b_integer_signed";
		ext = ::hilti::instruction::integer::SExt;
		}

	else
		{
		func = "LibBro::h2b_integer_unsigned";
		ext = ::hilti::instruction::integer::ZExt;
		}

	if ( itype->width() != 64 )
		{
		auto tmp = Builder()->addTmp("ext", ::hilti::builder::integer::type(64));
		Builder()->addInstruction(tmp, ext, val);
		val = tmp;
		}

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create(func), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Address* a)
	{
	auto val = arg1();
	auto dst = arg2();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_address"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Bool* b)
	{
	auto val = arg1();
	auto dst = arg2();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_bool"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Bytes* b)
	{
	auto val = arg1();
	auto dst = arg2();
	auto btype = arg3();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_bytes"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Double* d)
	{
	auto val = arg1();
	auto dst = arg2();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_double"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Enum* e)
	{
        auto val = arg1();
	auto dst = arg2();
	auto btype = arg3();

        auto type_idx = type_converter->TypeIndex(val->type(), btype);
        auto type_idx_hlt = ::hilti::builder::integer::create(type_idx);
        auto args = ::hilti::builder::tuple::create( { val, type_idx_hlt } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_enum"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::String* s)
	{
	auto val = arg1();
	auto dst = arg2();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_string"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Time* t)
	{
	auto val = arg1();
	auto dst = arg2();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_time"), args);
	setResult(true);
    }

void ValueConverter::visit(::hilti::type::Interval* i)
	{
	auto val = arg1();
	auto dst = arg2();

	auto args = ::hilti::builder::tuple::create( { val } );
	Builder()->addInstruction(dst, ::hilti::instruction::flow::CallResult,
				  ::hilti::builder::id::create("LibBro::h2b_interval"), args);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Tuple* t)
	{
	auto val = arg1();
	auto dst = arg2();
	auto ttype = ast::checkedCast<binpac::type::Tuple>(arg3());

	BroType* btype = nullptr;

	if ( _bro_type_hints.back() )
		btype = _bro_type_hints.back()->AsRecordType();

	else
		btype = type_converter->Convert(t->sharedPtr<::hilti::Type>(), ttype);

	auto hval = mbuilder->RuntimeHiltiToVal(val, btype);
	Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, hval);

	setResult(true);
	}

void ValueConverter::visit(::hilti::type::List* t)
	{
	auto val = arg1();
	auto dst = arg2();
	auto ltype = ast::checkedCast<binpac::type::List>(arg3());

	BroType* btype = nullptr;

	if ( _bro_type_hints.back() )
		btype = _bro_type_hints.back()->AsVectorType();

	else
		btype = type_converter->Convert(t->sharedPtr<::hilti::Type>(), ltype);

	auto result = mbuilder->ConversionBuilder()->ConvertHiltiToBro(val, btype);

	Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, result);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Vector* t)
	{
	auto val = arg1();
	auto dst = arg2();
	auto vtype = ast::checkedCast<binpac::type::Vector>(arg3());

	BroType* btype = nullptr;

	if ( _bro_type_hints.back() )
		btype = _bro_type_hints.back()->AsVectorType();

	else
		btype = type_converter->Convert(t->sharedPtr<::hilti::Type>(), vtype);

	auto result = mbuilder->ConversionBuilder()->ConvertHiltiToBro(val, btype);

	Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, result);
	setResult(true);
	}

void ValueConverter::visit(::hilti::type::Set* t)
	{
	auto val = arg1();
	auto dst = arg2();
	auto stype = ast::checkedCast<binpac::type::Set>(arg3());

	BroType* btype = nullptr;

	if ( _bro_type_hints.back() )
		btype = _bro_type_hints.back()->AsTableType();

	else
		btype = type_converter->Convert(t->sharedPtr<::hilti::Type>(), stype);

	auto result = mbuilder->ConversionBuilder()->ConvertHiltiToBro(val, btype);

	Builder()->addInstruction(dst, ::hilti::instruction::operator_::Assign, result);
	setResult(true);
	}

