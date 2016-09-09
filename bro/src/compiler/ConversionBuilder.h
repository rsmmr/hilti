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

class BroType;

namespace hilti {
class Expression;
class Type;
}

namespace binpac {
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
     * @param constant Should be true if it's guaramteed that \a val refers to a constant Bro value.
     *
     * @return An expression referencing the converted value.
     */
    shared_ptr<::hilti::Expression> ConvertBroToHilti(shared_ptr<::hilti::Expression> val,
                                                      const ::BroType* type, bool constant);

    /**
     * Generates code to convert a HILTI expression into a corresponding
     * Bro \a Val at runtime.
     *
     * @param val The value to convert.
     *
     * @param type The target Bro type to convert into.
     *
     * @param pac_type An optional BinPAC++ type that the value
     * corresponds to. If given, this might change some specifics of the
     * conversion.
     *
     * @return An expression referencing the converted value, which
     * correspond to a pointer to a Bro Val.
     */
    shared_ptr<::hilti::Expression> ConvertHiltiToBro(
        shared_ptr<::hilti::Expression> val, const ::BroType* type,
        shared_ptr<::binpac::Type> pac_type = nullptr);

    /**
     * XXX
     */
    std::shared_ptr<::hilti::Expression> FromAny(std::shared_ptr<::hilti::Expression> val,
                                                 const ::BroType* type);

    /**
     * XXX
     */
    std::shared_ptr<::hilti::Expression> ToAny(std::shared_ptr<::hilti::Expression> val,
                                               const ::BroType* type);

    /**
     * XXX
     */
    std::shared_ptr<::hilti::Expression> AnyBroType(std::shared_ptr<::hilti::Expression> val);

    /**
     * Generates code to create \a BroType object for at runtime.
     *
     * This is a bit odd but at runtime we sometimes need to have a
     * pointer to a BroType that we knew at compile-time but now can't
     * get other than by rebuilding it.
     *
     * If the method is called multiple times for the same Bro type, at
     * runtime it will always use the same (i.e., a single) instance.
     *
     * @param type The compile time \a BroType to recreate at runtime.
     *
     * @return A HILTI expression referencing the new \a BroType object.
     */
    shared_ptr<::hilti::Expression> CreateBroType(const ::BroType* type);

    /**
     * XXX
     */
    void MapType(const ::BroType* from, const ::BroType* to);

    /**
     * XXX
     */
    void BroRef(std::shared_ptr<::hilti::Expression> val);

    /**
     * XXX
     */
    void BroUnref(std::shared_ptr<::hilti::Expression> val);

    /**
     * XXX
     */
    void FinalizeTypes();

    /**
     * XXX
     */
    bool IsShadowedType(const BroType* type) const;

protected:
    typedef std::function<shared_ptr<::hilti::Expression>(shared_ptr<::hilti::Expression>,
                                                          const ::BroType* type)>
        build_conversion_function_callback;

    std::shared_ptr<::hilti::Expression> BuildConversionFunction(
        const char* tag, shared_ptr<::hilti::Expression> val, shared_ptr<::hilti::Type> valtype,
        shared_ptr<::hilti::Type> dsttype, const ::BroType* type,
        build_conversion_function_callback cb);

    typedef std::function<void(shared_ptr<::hilti::Expression>, const ::BroType* type)>
        build_create_type_callback;

    std::shared_ptr<::hilti::Expression> BuildCreateBroType(const char* tag, const ::BroType* type,
                                                            build_create_type_callback cb);

    std::shared_ptr<::hilti::Expression> PostponeBuildCreateBroType(const char* tag,
                                                                    const ::BroType* type,
                                                                    build_create_type_callback cb);

    std::shared_ptr<::hilti::Expression> AnyConversionFunction(const ::BroType* type);

    std::shared_ptr<::hilti::Expression> BroToHiltiBaseType(shared_ptr<::hilti::Expression> val,
                                                            const ::BroType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::EnumType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::TypeList* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::OpaqueType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::RecordType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::SubNetType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::TableType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::TypeType* type);
    std::shared_ptr<::hilti::Expression> BroToHilti(shared_ptr<::hilti::Expression> val,
                                                    const ::VectorType* type);

    std::shared_ptr<::hilti::Expression> HiltiToBroBaseType(shared_ptr<::hilti::Expression> val,
                                                            const ::BroType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::EnumType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::TypeList* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::OpaqueType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::RecordType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::SubNetType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::TableType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::TypeType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBro(shared_ptr<::hilti::Expression> val,
                                                    const ::VectorType* type);

    std::shared_ptr<::hilti::Expression> CreateBroTypeBase(const ::BroType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::EnumType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::TypeList* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::OpaqueType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::RecordType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::SubNetType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::TableType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::TypeType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::VectorType* type);
    std::shared_ptr<::hilti::Expression> CreateBroType(const ::FuncType* type);

private:
    std::shared_ptr<::hilti::Expression> BroInternalInt(std::shared_ptr<::hilti::Expression> val);
    std::shared_ptr<::hilti::Expression> BroInternalDouble(
        std::shared_ptr<::hilti::Expression> val);

    std::shared_ptr<::hilti::Expression> HiltiToBroFromStruct(shared_ptr<::hilti::Expression> val,
                                                              const ::RecordType* type);
    std::shared_ptr<::hilti::Expression> HiltiToBroFromTuple(shared_ptr<::hilti::Expression> val,
                                                             const ::RecordType* type);

    std::shared_ptr<::hilti::Expression> HiltiGlobalForType(const char* tag, const ::BroType* type);

    std::shared_ptr<::hilti::Expression> BuildCreateBroTypeInternal(const char* tag,
                                                                    const ::BroType* type,
                                                                    build_create_type_callback cb);

    typedef std::map<const ::BroType*, const ::BroType*> type_map;
    typedef std::map<const ::BroType*, std::pair<string, build_create_type_callback>>
        type_builder_map;
    typedef std::list<const ::BroType*> type_builder_list;

    bool _constant;
    type_builder_map postponed_types;
    shared_ptr<::binpac::Type> _pac_type;
};
}
}
}

#endif
