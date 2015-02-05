
#ifndef BINPAC_CODEGEN_CODEGEN_H
#define BINPAC_CODEGEN_CODEGEN_H

#include "../common.h"

namespace hilti {
    class Expression;
    class Module;
    class Type;
    class ID;

    namespace builder {
        class ModuleBuilder;
        class BlockBuilder;
    }

    namespace declaration {
        class Function;
    }
}

namespace binpac {

class CompilerContext;
class Options;

namespace codegen {
    class CodeBuilder;
    class TypeBuilder;
    class ParserBuilder;
    class Synchronizer;
}

class CodeGen : public ast::Logger
{
public:
    typedef std::list<shared_ptr<::hilti::ID>> id_list;

    /// Constructor.
    CodeGen(CompilerContext* ctx);
    virtual ~CodeGen();

    /// Returns the compiler context the code generator is used with.
    CompilerContext* context() const;

    /// Returns the options in effecty for code generation. This is a
    /// convienience method that just forwards to the current context.
    const Options& options() const;

    /// Compiles a BinPAC++ module into a HILTI module.
    ///
    /// module: The module to compile.
    ///
    /// debug: Debug level for the generated code.
    ///
    /// verify: True if the generated HILTI code should be run HILTI's verifier (usually a good idea...)
    ///
    /// Returns: The HILTI module, or null if an error occured.
    shared_ptr<hilti::Module> compile(shared_ptr<Module> module);

    /// Returns the currently compiled module, or null if none.
    shared_ptr<Module> module() const;

    /// Returns the current HILTI module builder.
    shared_ptr<hilti::builder::ModuleBuilder> moduleBuilder() const;

    /// Returns the current HILTI block builder.
    shared_ptr<hilti::builder::BlockBuilder> builder() const;

    /// Returns the type of the currently parsed unit. The method must only
    /// be called when parsing is in progress.
    shared_ptr<type::Unit> unit() const;

    /// Returns the HILTI epxression resulting from evaluating a BinPAC
    /// expression. The method adds the code to the current HILTI block.
    ///
    /// expr: The expression to evaluate.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    ///
    /// Returns: The computed HILTI expression.
    shared_ptr<hilti::Expression> hiltiExpression(shared_ptr<Expression> expr, shared_ptr<Type> coerce_to = nullptr);

    /// Compiles a BinPAC statement into HILTI. The method adds the code to
    /// current HILTI block.
    ///
    /// stmt: The statement to compile
    void hiltiStatement(shared_ptr<Statement> stmt);

    /// Converts a BinPAC type into its corresponding HILTI type.
    ///
    /// type: The type to convert.
    ///
    /// deps: If given, any types that will need to be imported will be
    /// added to the list with their qualified IDs.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Type> hiltiType(shared_ptr<Type> type, id_list* deps = nullptr);

    /// Returns the HILTI type associated with a BinPAC++ integer type.
    ///
    /// width: The width of the interger type.
    ///
    /// signed_: True if creating a signed integer type, false for unsigned.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Type> hiltiTypeInteger(int width, bool signed_);

    /// Returns the HILTI type associated with a BinPAC++ optional type.
    ///
    /// t: The type the optional is wrapping, or null for a wildcard type.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Type> hiltiTypeOptional(shared_ptr<Type> t);

    /// Returns a HILTI constant expression of BinPAC++ integer type.
    ///
    /// value: The constant's value.
    ///
    /// signed_: True if creating a signed integer constant, false for
    /// unsigned.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Expression> hiltiConstantInteger(int value, bool signed_);

    /// Returns a HILTI constant expression of BinPAC++ optional type.
    ///
    /// value: The optional's value, or null for unset.
    ///
    /// Returns: The HILTI type.
    shared_ptr<hilti::Expression> hiltiConstantOptional(shared_ptr<Expression> value = 0);

    /// Returns the HILTI name that is associated with a BinPAC++ type. This
    /// is curently defined only for units.
    ///
    /// type: The type to return the name for, which must be a unit type.
    ///
    /// Returns: The HILTI ID>
    shared_ptr<hilti::ID> hiltiTypeID(shared_ptr<Type> type);

    /// Coerces a HILTI expression of one BinPAC type into another. The
    /// method assumes that the coercion is legel.
    ///
    /// expr: The HILTI expression to coerce.
    ///
    /// src: The BinPAC type that \a expr currently has.
    ///
    /// dst: The BinPAC type that \a expr is to be coerced into.
    ///
    /// Returns: The coerced expression.
    shared_ptr<hilti::Expression> hiltiCoerce(shared_ptr<hilti::Expression> expr, shared_ptr<Type> src, shared_ptr<Type> dst);

    /// Generates the code to synchronized streaming at a production. The
    /// production must indicate support for synchronization via
    /// canSynchronize(). The code throws a XXX exception when
    /// synchronization failed.
    ///
    /// data: The current input bytes object.
    ///
    /// cur: The current input position in \a data
    ///
    /// Returns: The new input position.
    shared_ptr<hilti::Expression> hiltiSynchronize(shared_ptr<Production> p, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> cur);

    /// Returns the default value for instances of a BinPAC type that aren't
    /// further intiailized.
    ///
    /// type: The type to convert.
    ///
    /// null_on_default: If true, returns null if the type uses the HILTI
    /// default as its default.
    ///
    /// can_be_unset: If true, the method can return null to indicate that a
    /// value should be left unset by default. If false, there will always a
    /// value returned (unless \a null_on_default is set).
    ///
    /// Returns: The HILTI value, or null for HILTI's default.
    shared_ptr<hilti::Expression> hiltiDefault(shared_ptr<Type> type, bool null_on_default, bool can_be_unset);

    /// Turns a BinPAC ID into a HILTI ID.
    ///
    /// id: The ID to convert.
    ///
    /// qualify: If true, the ID will be scoped acoording to the current
    /// module. That means that if it's parent module is a different one, a
    /// corresponding namespace will be inserted.
    ///
    /// Returns: The converted ID.
    shared_ptr<hilti::ID> hiltiID(shared_ptr<ID> id, bool qualify = false);

    /// Return the parse function for a unit type. If it doesn't exist yet,
    /// it will be created.
    ///
    /// u: The unit to generate the parser for.
    ///
    /// Returns: The generated HILTI function with the parsing code.
    shared_ptr<hilti::Expression> hiltiParseFunction(shared_ptr<type::Unit> u);

    /// Generates the externally visible functions for parsing a unit type.
    ///
    /// u: The unit type to export via functions.
    void hiltiExportParser(shared_ptr<type::Unit> unit);

    /// Generates the implementation of unit-embedded hooks.
    ///
    /// u: The unit type to generate the hooks for.
    void hiltiUnitHooks(shared_ptr<type::Unit> unit);

    /// Generates code to execute the hooks associated with an unit item.
    /// This must only be called while a unit is being parsed.
    ///
    /// item: The item.
    ///
    /// self: The expression to pass as the hook's \a self argument. Must
    /// match the type of the unit that \a item is part of.
    void hiltiRunFieldHooks(shared_ptr<type::unit::Item> item, shared_ptr<hilti::Expression> self);

    // Returns the HILTI struct type for a unit's parse object.
    //
    /// u: The unit to return the type for.
    ///
    /// Returns: The type.
    shared_ptr<hilti::Type> hiltiTypeParseObject(shared_ptr<type::Unit> unit);

    /// Returns the HILTI reference type for a unit's parse object.
    ///
    /// u: The unit to return the type for.
    ///
    /// Returns: The type.
    shared_ptr<hilti::Type> hiltiTypeParseObjectRef(shared_ptr<type::Unit> unit);

    /// Adds a global constant with a unit's auxiliary type information.
    ///
    /// unit: The unit type.
    ///
    /// htype:: The HILTI struct type that corresponds to the unit type.
    ///
    /// Returns: An expression referencing the global constant.
    shared_ptr<hilti::Expression> hiltiAddParseObjectTypeInfo(shared_ptr<Type> unit);

    /// Adds a global type to the current module. The method may augment the
    /// type with further meta-information as necessary.
    ///
    /// id: The ID to associate the type with.
    ///
    /// htype: The HILTI type to add to the module.
    ///
    /// btype: The BinPAC++ type that htype corresponds to.
    void hiltiAddType(shared_ptr<ID> id, shared_ptr<hilti::Type> htype, shared_ptr<Type> btype);

    /// Adds an external implementation of a unit hook.
    ///
    /// id: The hook's ID (full path).
    ///
    /// hook: The hook itself.
    void hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook);

    /// Adds a function to the current module.
    ///
    /// function: The function.
    shared_ptr<hilti::declaration::Function> hiltiDefineFunction(shared_ptr<Function> func, bool declare_only = false, const string& scope = "");

    /// Adds a function to the current module.
    ///
    /// function: An expression referencing the function.
    shared_ptr<hilti::declaration::Function> hiltiDefineFunction(shared_ptr<expression::Function> func, bool declare_only = false);

    /// Calls a function.
    ///
    /// func: The function to call.
    ///
    /// args: The arguments.
    ///
    /// cookie: If we are currently parsing, the user's cookie; otherwise null.
    ///
    /// Returns: The function's result, or null for void functions.
    shared_ptr<hilti::Expression> hiltiCall(shared_ptr<expression::Function> func, const expression_list& args, shared_ptr<hilti::Expression> cookie);

    /// Returns a HILTI expression referencing the current parser object
    /// (assuming parsing is in process; if not aborts());
    ///
    shared_ptr<hilti::Expression> hiltiSelf();
    /// Returns a HILTI expression referencing the current user cookie
    /// (assuming parsing is in process; if not aborts());
    shared_ptr<hilti::Expression> hiltiCookie();

    /// Returns the HILTI type for the cookie argument.
    shared_ptr<hilti::Type> hiltiTypeCookie();

    /// Returns the HILTI-level name for a function.
    shared_ptr<hilti::ID> hiltiFunctionName(shared_ptr<binpac::Function> func, const string& scope = "");

    /// Returns the HILTI-level name for a function.
    shared_ptr<hilti::ID> hiltiFunctionName(shared_ptr<expression::Function> func);

    /// Binds the $$ identifier to a given value.
    ///
    /// val: The value.
    void hiltiBindDollarDollar(shared_ptr<hilti::Expression> val);

    /// Unbinds the $$ identifier.
    void hiltiUnbindDollarDollar();

    /// Returns the new() function that instantiates a new parser object. In
    /// addition to the type parameters, the returned function receives two
    /// parameters: a ``hlt_sink *`` with the sink the parser is connected to
    /// (NULL if none); and a ``bytes`` object with the MIME type associated
    /// with the input (empty if None). The function also runs the %init hook.
    ///
    /// unit: The unit type to return the function for.
    shared_ptr<hilti::Expression> hiltiFunctionNew(shared_ptr<type::Unit> unit);

    /// Converts an enum value from one type into an enum value of another by
    /// casting the integer value over.
    shared_ptr<hilti::Expression> hiltiCastEnum(shared_ptr<hilti::Expression> val, shared_ptr<hilti::Type> dst);

    /// Writes a new chunk of data into a field's sinks.
    ///
    /// field: The field.
    ///
    /// data: The data to write into the sink.
    void hiltiWriteToSinks(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> data);

    /// Writes a new chunk of data into a sink.
    ///
    /// sink: The sink to write to.
    ///
    /// data: The data to write into the sink.
    void hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> data);

    /// Writes a new chunk of data into a sink.
    ///
    /// begin: Start iterator of data to write.
    ///
    /// end: End iterator of data to write.
    ///
    /// data: The data to write into the sink.
    void hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> begin, shared_ptr<hilti::Expression> end);

    /// Applies transformation attributes (such as \a convert) to a value, if
    /// present.
    ///
    /// val: The value.
    ///
    /// attrs: The set of attributes to apply. The method filters for the
    /// ones supported and ignores all other ones.
    ///
    /// Returns: The transformed value, which may be just \c val if no
    /// transformation applied.
    shared_ptr<hilti::Expression> hiltiApplyAttributesToValue(shared_ptr<hilti::Expression> val, shared_ptr<AttributeSet> attrs);

    /// XXX
    void hiltiImportType(shared_ptr<ID> id, shared_ptr<Type> t);

    /// Converts an expression of type BinPAC::ByteOrder into a HILTI value of type Hilti::ByteOrder.
    ///
    /// expr: The expression to convert.
    ///
    /// Returns: The equivalent HILTI byte order.
    shared_ptr<hilti::Expression> hiltiByteOrder(shared_ptr<Expression> expr);

    /// Converts an expression of type BinPAC::Charset into a HILTI value of type Hilti::Charset.
    ///
    /// expr: The expression to convert.
    ///
    /// Returns: The equivalent HILTI character set.
    shared_ptr<hilti::Expression> hiltiCharset(shared_ptr<Expression> expr);

    /// XXX
    shared_ptr<hilti::Expression> hiltiExtractsBitsFromInteger(shared_ptr<hilti::Expression> value, shared_ptr<Type> type, shared_ptr<Expression> border, shared_ptr<hilti::Expression> lower, shared_ptr<hilti::Expression> upper);

    /// Extracts the value for a unit field. If a field is not set, but a
    /// default has been preset, that default will be returned.
    ///
    /// unit: The unit instance to extract the field from.
    ///
    /// field: The field to extract.
    ///
    /// default_: The default to return if the field is neither set nor a
    /// default has been preset. If not given, an unset field will throw an
    /// exception. If given, this behaves like the HILTI \c
    /// struct.get_default instruction.
    ///
    /// Returns: The field's current value.
    shared_ptr<hilti::Expression> hiltiItemGet(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, shared_ptr<hilti::Expression> default_ = nullptr);

    /// Extracts the value for a unit field.
    ///
    /// unit: The unit instance to extract the field from.
    ///
    /// field: The ID referencing the field to extract.
    ///
    /// default_: The default to return if the field is not set. If not
    /// given, an unset field will throw an exception.
    ///
    /// Returns: The field's current value.
    shared_ptr<hilti::Expression> hiltiItemGet(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field, shared_ptr<::hilti::Type> ftype, shared_ptr<hilti::Expression> default_ = nullptr);

    /// Extracts the value for a unit field.
    ///
    /// unit: The unit instance to extract the field from.
    ///
    /// field: The ID referencing the field to extract.
    ///
    /// default_: The default to return if the field is not set. If not
    /// given, an unset field will throw an exception.
    ///
    /// Returns: The field's current value.
    shared_ptr<hilti::Expression> hiltiItemGet(shared_ptr<hilti::Expression> unit, const string& field, shared_ptr<::hilti::Type> ftype, shared_ptr<hilti::Expression> default_ = nullptr);

    /// Sets the value for a unit field.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The field to set.
    ///
    /// value: The value to set the field to.
    void hiltiItemSet(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, shared_ptr<hilti::Expression> value);

    /// Sets the value for a unit field.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The name of the field to set.
    ///
    /// value: The value to set the field to.
    void hiltiItemSet(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field, shared_ptr<hilti::Expression> value);

    /// Sets the value for a unit field.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The name of the field to set.
    ///
    /// value: The value to set the field to.
    void hiltiItemSet(shared_ptr<hilti::Expression> unit, const string& field, shared_ptr<hilti::Expression> value);

    /// Sets the default value for a unit field. The is what hiltiItemGet()
    /// will return if not value has been explicitly set.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The field to set.
    ///
    /// def: The default to set the field to.
    void hiltiItemPresetDefault(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, shared_ptr<hilti::Expression> def);

    /// Unsets the value for a unit field.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The field to set.
    void hiltiItemUnset(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field);

    /// Unsets the value for a unit field.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The name of the field to set.
    void hiltiItemUnset(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field);

    /// Unsets the value for a unit field.
    ///
    /// unit: The unit instance to set the field in.
    ///
    /// field: The name of the field to set.
    void hiltiItemUnset(shared_ptr<hilti::Expression> unit, const string& field);

    /// Checks if a unit field has a value set for it.
    ///
    /// unit: The unit instance to check the field for.
    ///
    /// field: The field to check.
    ///
    /// Returns: A boolean HILTI expression being true if the field has a value currently.
    shared_ptr<hilti::Expression> hiltiItemIsSet(shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field);

    /// Checks if a unit field has a value set for it.
    ///
    /// unit: The unit instance to check the field for.
    ///
    /// field: The name of the field to check.
    ///
    /// Returns: A boolean HILTI expression being true if the field has a value currently.
    shared_ptr<hilti::Expression> hiltiItemIsSet(shared_ptr<hilti::Expression> unit, shared_ptr<ID> field);

    /// Checks if a unit field has a value set for it.
    ///
    /// unit: The unit instance to check the field for.
    ///
    /// field: The name of the field to check.
    ///
    /// Returns: A boolean HILTI expression being true if the field has a value currently.
    shared_ptr<hilti::Expression> hiltiItemIsSet(shared_ptr<hilti::Expression> unit, const string& field);

    /// Computes the path to a field inside the unit structure in terms of
    /// indices followed. In the simplest cast of a field being stored at the
    /// top-level of a unit's underlying struct, this will return just \c {
    /// idx }. If a field is stored nested more deeply (like inside a union),
    /// the returned list will list the successive indices to get there.
    /// Note, the indices correspond to what \c {struct,union}::index()
    /// return, not the physical ones (which for a union would always be
    /// zero).
    ///
    /// unit: The unit type that the field is part of.
    ///
    /// f: The field.
    ///
    /// Returns: A HILTI tuple of int<64> representing the path to \a f.
    shared_ptr<hilti::Expression> hiltiItemComputePath(shared_ptr<hilti::Type> unit, shared_ptr<binpac::type::unit::Item> f);

    /// Enable BinPAC++-specific HILTI optimizations
    void enableBinPACOptimizations();
    
private:
    enum HiltiItemOp { GET, GET_DEFAULT, SET, PRESET_DEFAULT, UNSET, IS_SET };
    shared_ptr<hilti::Expression> _hiltiItemOp(HiltiItemOp i, shared_ptr<hilti::Expression> unit, shared_ptr<type::unit::Item> field, const std::string& fname, shared_ptr<::hilti::Type> ftype, shared_ptr<hilti::Expression> addl_op);

    CompilerContext* _ctx;
    bool _compiling = false;
    shared_ptr<Module> _module = nullptr;
    std::map<string, shared_ptr<Type>> _imported_types;

    shared_ptr<hilti::builder::ModuleBuilder> _mbuilder;

    unique_ptr<codegen::CodeBuilder>     _code_builder;
    unique_ptr<codegen::ParserBuilder>   _parser_builder;
    unique_ptr<codegen::TypeBuilder>     _type_builder;
    unique_ptr<codegen::Synchronizer>    _synchronizer;
    unique_ptr<Coercer>     _coercer;
    
    bool _bpOpts = false;
};

}

#endif
