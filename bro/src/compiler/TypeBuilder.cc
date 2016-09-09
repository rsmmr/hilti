
#include <Type.h>
#include <module_util.h>
#undef List

#include <hilti/builder/nodes.h>
#include <util/util.h>

#include "Compiler.h"
#include "TypeBuilder.h"

using namespace bro::hilti::compiler;

TypeBuilder::TypeBuilder(class ModuleBuilder* mbuilder) : BuilderBase(mbuilder)
{
}

shared_ptr<hilti::Type> TypeBuilder::Compile(const ::BroType* type)
{
    auto cached = Compiler()->LookupCachedCustomBroType(type);

    if ( cached.first )
        return cached.first;

    switch ( type->Tag() ) {
    case TYPE_ADDR:
    case TYPE_ANY:
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
    case TYPE_VOID:
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
        return ::hilti::builder::address::type();

    case TYPE_ANY:
        return ::hilti::builder::reference::type(::hilti::builder::type::byName("LibBro::BroAny"));

    case TYPE_BOOL:
        return ::hilti::builder::boolean::type();

    case TYPE_COUNT:
    case TYPE_COUNTER:
        return ::hilti::builder::integer::type(64);

    case TYPE_DOUBLE:
        return ::hilti::builder::double_::type();

    case TYPE_INT:
        return ::hilti::builder::integer::type(64);

    case TYPE_INTERVAL:
        return ::hilti::builder::interval::type();

    case TYPE_PATTERN:
        return ::hilti::builder::reference::type(::hilti::builder::regexp::type());

    case TYPE_PORT:
        return ::hilti::builder::port::type();

    case TYPE_STRING:
        return ::hilti::builder::reference::type(::hilti::builder::bytes::type());

    case TYPE_TIME:
        return ::hilti::builder::time::type();

    case TYPE_VOID:
        return ::hilti::builder::void_::type();

    default:
        Error("TypeBuilder: cannot be reached", type);
    }

    // Cannot be reached.
    return nullptr;
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::EnumType* type)
{
    auto idx = type->GetName().size() ? string(type->GetName()) : ::util::fmt("enum_%p", type);

    auto sm = ::extract_module_name(idx.c_str());
    auto sv = ::extract_var_name(idx.c_str());
    idx = ::util::fmt("%s::%s", sm, sv);

    if ( auto t = ModuleBuilder()->lookupNode("enum-type", idx) )
        return ast::rtti::checkedCast<::hilti::Type>(t);

    ::hilti::builder::enum_::label_list labels;

    for ( auto i : type->Names() ) {
        auto label = i.first;
        auto val = i.second;

        auto name = ::util::strreplace(label, "::", "_");
        auto id = ::hilti::builder::id::node(name);

        labels.push_back(std::make_pair(id, val));
    }

    auto etype = ::hilti::builder::enum_::type(labels);
    ModuleBuilder()->cacheNode("enum-type", idx, etype);
    ModuleBuilder()->addType(idx, etype);
    return etype;
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::FileType* type)
{
    return ::hilti::builder::reference::type(::hilti::builder::file::type());
}

shared_ptr<::hilti::type::Function> TypeBuilder::FunctionType(const ::FuncType* type)
{
    auto byield = type->YieldType();
    auto bargs = type->Args();

    std::shared_ptr<::hilti::Type> hyield;

    if ( byield && byield->Tag() != TYPE_VOID )
        hyield = HiltiType(byield);
    else
        hyield = ::hilti::builder::void_::type();

    auto hresult = ::hilti::builder::function::result(hyield, false);
    auto hargs = ::hilti::builder::function::parameter_list();

    for ( int i = 0; i < bargs->NumFields(); i++ ) {
        auto name = bargs->FieldName(i);
        auto ftype = bargs->FieldType(i);
        auto htype = HiltiType(ftype);
        auto def =
            bargs->FieldDefault(i) ? HiltiValue(bargs->FieldDefault(i), ftype, true) : nullptr;

        auto param = ::hilti::builder::function::parameter(name, htype, false, def);
        hargs.push_back(param);
    }

    switch ( type->Flavor() ) {
    case FUNC_FLAVOR_FUNCTION:
        return ::hilti::builder::function::type(hresult, hargs);

    case FUNC_FLAVOR_EVENT:
        return ::hilti::builder::hook::type(hresult, hargs);

    case FUNC_FLAVOR_HOOK:
        hyield = ::hilti::builder::boolean::type();
        hresult = ::hilti::builder::function::result(hyield, false);
        return ::hilti::builder::hook::type(hresult, hargs);

    default:
        InternalError("unknown function flavor", type);
    }

    InternalError("cannot be reached", type);
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::FuncType* type)
{
    return ::hilti::builder::type::byName("LibBro::BroVal");
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::OpaqueType* type)
{
    return ::hilti::builder::caddr::type();
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::RecordType* type)
{
    auto idx = type->GetName().size() ? string(type->GetName()) : ::util::fmt("record_%p", type);

    auto sm = ::extract_module_name(idx.c_str());
    auto sv = ::extract_var_name(idx.c_str());
    idx = ::util::fmt("%s::%s", sm, sv);

    if ( auto t = ModuleBuilder()->lookupNode("struct-type", idx) )
        return ast::rtti::checkedCast<::hilti::Type>(t);

    ::hilti::builder::struct_::field_list fields;

    for ( int i = 0; i < type->NumFields(); i++ ) {
        auto bdef = type->FieldDefault(i);

        auto name = type->FieldName(i);
        auto ftype = type->FieldType(i);
        auto htype = HiltiType(ftype);
        auto optional = type->FieldDecl(i)->FindAttr(ATTR_OPTIONAL);
        auto def = bdef ? HiltiValue(bdef, ftype, true) :
                          (optional ? nullptr : HiltiDefaultInitValue(ftype));
        auto hf = ::hilti::builder::struct_::field(name, htype, def);

        fields.push_back(hf);
    }

    auto stype = ::hilti::builder::struct_::type(fields);
    auto rtype = ::hilti::builder::reference::type(stype);

    ModuleBuilder()->cacheNode("struct-type", idx, rtype);
    ModuleBuilder()->addType(idx, stype);
    return rtype;
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::SubNetType* type)
{
    return ::hilti::builder::network::type();
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::TableType* type)
{
    std::shared_ptr<::hilti::Type> idx;

    auto itypes = type->IndexTypes();

    if ( itypes->length() == 1 )
        idx = HiltiType((*itypes)[0]);
    else {
        ::hilti::builder::type_list types;

        loop_over_list(*itypes, i) types.push_back(HiltiType((*itypes)[i]));

        idx = ::hilti::builder::tuple::type(types);
    }

    if ( type->IsSet() )
        return ::hilti::builder::reference::type(::hilti::builder::set::type(idx));

    auto vtype = HiltiType(type->YieldType());
    return ::hilti::builder::reference::type(::hilti::builder::map::type(idx, vtype));
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::TypeList* type)
{
    Error("no support yet for compiling TypeList", type);
    return nullptr;
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::TypeType* type)
{
    return ::hilti::builder::type::byName("LibBro::BroType");
}

std::shared_ptr<::hilti::Type> TypeBuilder::Compile(const ::VectorType* type)
{
    auto etype = HiltiType(type->YieldType());
    return ::hilti::builder::reference::type(::hilti::builder::vector::type(etype));
}
