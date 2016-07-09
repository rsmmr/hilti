
#ifndef HILTI_CODEGEN_CODEGEN_H
#define HILTI_CODEGEN_CODEGEN_H

#include <list>

#include "../type.h"
#include "../visitor.h"
#include "common.h"
#include "symbols.h"
#include "util.h"

namespace hilti {

class Module;
class Type;
class Expression;
class Variable;
class LogComponent;
class CompilerContext;
class Options;

namespace statement {
class Instruction;
}
namespace passes {
class Collector;
}

namespace codegen {

class ABI;

namespace util {
class IRInserter;
}

struct TypeInfo;

/// Namespace for symbols that corresponds to, and must with, element defined
/// at the C layer in libhilti.
namespace hlt {
/// Fields in %hlt.execution_context.
enum ExecutionContext { Globals = 13 };

/// Fields in %hlt.exception.
enum Exception { Name = 0 };
}

class Coercer;
class Loader;
class Storer;
class Unpacker;
class Packer;
class FieldBuilder;
class StatementBuilder;
class TypeBuilder;
class TypeInfoBuilder;
class DebugInfoBuilder;
class PointerMap;

/// The IRBuilder used by the code generator.
typedef util::IRBuilder IRBuilder;

/// Central code generator. This class coordinates the code generatio and
/// provides lots of helpers to create LLVM elements. Most of the knowledge
/// about how the generated LLVM code looks like is here. However, various
/// object specfific tasks are outsourced to other visitors, such as
/// loads/stores and per-statement code generation (but these other classes
/// then tend to leverage the code generator again for their work).
class CodeGen : public ast::Logger {
public:
    /// Constructor.
    ///
    /// ctx: The compiler context the code geneator is used with.
    ///
    /// libdirs: Path where to find library modules that the code generator
    /// may need.
    CodeGen(CompilerContext* ctx, const path_list& libdirs);
    virtual ~CodeGen();

    /// Returns the compiler context the code generator is used with.
    ///
    CompilerContext* context() const;

    /// Returns the options in effecty for code generation. This is a
    /// convienience method that just forwards to the current context.
    const Options& options() const;

    /// Main entry method for compiling a HILTI AST into an LLVM module.
    ///
    /// hltmod: The module to compile.
    ///
    /// verify: True if the resulting LLVM module should be checked for
    /// correctness. This should normally be on except for debugging.
    ///
    /// debug: The debug level to activate for code generation. The higher, the
    /// more debugging code will be compiled in. Zero disables all debugging.
    ///
    /// profile: The profiling level to activate for code generation. The
    /// higher, the more profiling code will be compiled in. Zero disables all
    /// profiling.
    ///
    /// Returns: The compiled LLVM module, or null if errors are encountered.
    std::unique_ptr<llvm::Module> generateLLVM(shared_ptr<hilti::Module> hltmod);

    /// Returns the LLVM context to use with all LLVM calls.
    llvm::LLVMContext& llvmContext();

    /// Returns the LLVM data layout for the currently being built module.
    llvm::DataLayout* llvmDataLayout()
    {
        return _data_layout;
    }

    /// Returns the LLVM builder to use for inserting code at the current
    /// location. For each LLVM function being built, a stack of builder is
    /// maintained and manipulated via pushBuilder() and popBuilder().
    IRBuilder* builder() const;

    /// Returns the LLVM function currently being generated.
    llvm::Function* function() const
    {
        return _functions.back()->function;
    }

    /// Pushes a new LLVM function on to the stack of function being
    /// generated. This must be called when a function is created and will
    /// then be returned by function() as long as no further functions are
    /// pushed. At the end of code generation for a function, popFunction()
    /// must be called.
    ///
    /// function: The function to push onto the stack.
    ///
    /// push_builder: If true, the methods adds an entry block to the
    /// function, wraps it into an IRBuilder, and makes that with
    /// current builder() with pushBuilder().
    ///
    /// abort_on_excpt: If true, exceptions in this function will immeidately
    /// abort execution rather than triggering normal handling.
    ///
    /// cc: The function's calling convetion; DEFAULT means "not further
    /// specified".
    llvm::Function* pushFunction(llvm::Function* function, bool push_builder = true,
                                 bool abort_on_excpt = false, bool is_init_func = false,
                                 type::function::CallingConvention = type::function::DEFAULT);

    /// Removes the current LLVM function from the stack of function being
    /// generated. Calls to this function must match with those to
    /// pushFunction(). The method also normalizes the function's code and
    /// adds furthter cleanup infrastructure0 as neccessary.
    void popFunction();

    /// Returns true of no code has yet been added to the current function().
    bool functionEmpty() const;

    /// Returns the LLVM block associated with the current builder().
    llvm::BasicBlock* block() const;

    /// Pushes a new builder onto the stack of builders associated with the
    /// current function. A corresponding popBuilder() must eventually be
    /// called.
    ///
    /// builder: The builder.
    ///
    /// Returns: The LLVM IRBuilder.
    IRBuilder* pushBuilder(IRBuilder* builder);

    /// Creates a new LLVM block and pushes a corresponding builder onto the
    /// stack of builders associated with the current function. A
    /// corresponding popBuilder() must eventually be called.
    ///
    /// name: The name of the new block, or empty if none. Note that it might
    /// be further mangled and made unique. If empty, the block will be
    /// anonymous
    ///
    /// reuse: If true and we have already created a builder of the given name
    /// within the same function, that will be used.
    ///
    /// Returns: The LLVM IRBuilder.
    IRBuilder* pushBuilder(string name = "", bool reuse = false);

    /// Removes the top-most builder from the stack of builders associated
    /// with the current LLVM function(). Calls to this method must match
    /// those to pushBuilder().
    void popBuilder();

    /// Creates a new LLVM block along with a corresponding builder. It does
    /// however not push it onto stack of builders associated with the current
    /// function.
    ///
    /// name: The name of the new block, or empty if none. Note that it might
    /// be further mangled and made unique. If empty, the block will be
    /// anonymous
    ///
    /// reuse: If true and we have already created a builder of the given name
    /// within the same function, that will be returned.
    ///
    /// create: If false, the builder will in facy not be created if it
    /// doesn't exist already. If you want to just check whether a builder
    /// exists, better to use haveBuilder().
    ///
    /// Returns: The LLVM IRBuilder.
    IRBuilder* newBuilder(string name = "", bool reuse = false, bool create = true);

    /// Creates a new LLVM builder with a given block as its insertion point.
    /// It does however not push it onto stack of builders associated with the
    /// current function.
    ///
    /// block: The block to set as insertion point.
    ///
    /// insert_at_beginning: If true, the insertion point is set to the
    /// beginning of the block; otherwise to the end.
    ///
    /// Returns: The LLVM IRBuilder.
    IRBuilder* newBuilder(llvm::BasicBlock* block, bool insert_at_beginning = false);

    /// Mangles the ID of a global. This is similar to \a util::mangle but
    /// takes module names into a account by adding either the module given as
    /// prefix, or the current module if not given.
    string mangleGlobal(shared_ptr<ID> id, shared_ptr<Module> mod = nullptr, string prefix = "",
                        bool internal = false);

    /// XXXX
    string scopedNameGlobal(Variable* var) const;

    /// Returns the builder associated with a block label. This first this is
    /// called for a given pair label/function, a new builder is created and
    /// returned. Subsequent calls for the same pair will then return the same
    /// builder.
    ///
    /// id: The label.
    ///
    /// Returns: The builder.
    IRBuilder* builderForLabel(const string& name);

    /// Checks we have already created a builder of the given name with any of
    /// newBuilder() / pushBuilder() / builderForLabel(). If so, that's
    /// returned.
    IRBuilder* haveBuilder(const string& name);

    /// Caches an LLVM value for later reuse. The value is identified by two
    /// string keys that can be chosen by the caller. lookupCachedValue() will
    /// return the value if called with the same arguments.
    ///
    /// component: A string identifying a (fuzzily defined) component
    /// associated with the value.
    ///
    /// key: A string identifying the specific value being cached relative to
    /// the \a component.
    ///
    /// val: The value to cachen.
    ///
    /// Returns: \a val.
    llvm::Value* cacheValue(const string& component, const string& key, llvm::Value* val);

    /// Looks up a previously cached LLVM value. If the arguments match what
    /// was passed to cacheValue(), the corresponding value is returned.
    ///
    /// component: A string identifying a (fuzzily defined) component
    /// associated with the value.
    ///
    /// key: A string identifying the specific value being cached relative to
    /// the \a component.
    ///
    /// Returns: The cache value if found, or null if not.
    llvm::Value* lookupCachedValue(const string& component, const string& key);

    /// Caches an LLVM constant for later reuse. The constant is identified by two
    /// string keys that can be chosen by the caller. lookupCachedConstant() will
    /// return the constant if called with the same arguments.
    ///
    /// component: A string identifying a (fuzzily defined) component
    /// associated with the constant.
    ///
    /// key: A string identifying the specific constant being cached relative to
    /// the \a component.
    ///
    /// val: The constant to cachen.
    ///
    /// Returns: \a val.
    llvm::Constant* cacheConstant(const string& component, const string& key, llvm::Constant* val);

    /// Looks up a previously cached LLVM constant. If the arguments match what
    /// was passed to cacheConstant(), the corresponding constant is returned.
    ///
    /// component: A string identifying a (fuzzily defined) component
    /// associated with the constant.
    ///
    /// key: A string identifying the specific constant being cached relative to
    /// the \a component.
    ///
    /// Returns: The cache constant if found, or null if not.
    llvm::Constant* lookupCachedConstant(const string& component, const string& key);

    /// Caches an LLVM type for later reuse. The type is identified by two
    /// string keys that can be chosen by the caller. lookupCachedTypes() will
    /// return the type if called with the same arguments.
    ///
    /// component: A string identifying a (fuzzily defined) component
    /// associated with the type.
    ///
    /// key: A string identifying the specific type being cached relative to
    /// the \a component.
    ///
    /// val: The type to cachen.
    ///
    /// Returns: \a val.
    llvm::Type* cacheType(const string& component, const string& key, llvm::Type* type);

    /// Looks up a previously cached LLVM type. If the arguments match what
    /// was passed to cacheType(), the corresponding type is returned.
    ///
    /// component: A string identifying a (fuzzily defined) component
    /// associated with the type.
    ///
    /// key: A string identifying the specific type being cached relative to
    /// the \a component.
    ///
    /// Returns: The cache type if found, or null if not.
    llvm::Type* lookupCachedType(const string& component, const string& key);

    /// Constracts a unique name. This is intended for creating variable
    /// names. It remembers names already created and will return a new one
    /// each time.
    ///
    /// component: A string identifying a (fuzzily defined) component asking
    /// for the name.
    ///
    /// str: A string interpreted relative to \a component that will go into
    /// the returned value.
    ///
    /// Returns: A unique string.
    string uniqueName(const string& component, const string& str);

    /// Looks up a module-global type by its name. The type must exist,
    /// execution will abort if not.
    ///
    /// name: The fully qualified name of the type.
    shared_ptr<Type> typeByName(const string& name);

    /// Returns the ABI module to use for the current compilation.
    ABI* abi() const
    {
        return _abi.get();
    }

    /// Adds a comment just before the next instruction that will be printed
    /// out by the AssemblyAnnotationWriter.
    void llvmInsertComment(const string& comment);

    /// Returns the LLVM module currently being built.
    llvm::Module* llvmModule() const
    {
        return _module.get();
    }

    /// Returns the LLVM module currently being compiled.
    shared_ptr<hilti::Module> hiltiModule() const
    {
        return _hilti_module;
    }

    /// Returns the LLVM value associated with a function's local variable.
    ///
    /// name: The name of the local to access.
    ///
    /// Returns: An LLVM value representing a \a pointer to the local.
    llvm::Value* llvmLocal(const string& name);

    /// Returns the LLVM value associated with a module's global variable.
    ///
    /// var: The global variable.
    ///
    /// Returns: An LLVM value representing a \a pointer to the local.
    llvm::Value* llvmGlobal(shared_ptr<Variable> var);

    /// Returns the LLVM value associated with a module's global variable.
    ///
    /// var: The global variable.
    ///
    /// Returns: An LLVM value representing a \a pointer to the local.
    llvm::Value* llvmGlobal(Variable* var);

    /// Returns the LLVM value associated with a function's parameter.
    ///
    /// param: The parameter.
    ///
    /// Returns: An LLVM value representing the function parameter.
    llvm::Value* llvmParameter(shared_ptr<type::function::Parameter> param);

    /// Returns the LLVM function that corresponds to a HILTI function. If
    /// function hasn't been generated yet, this adds a corresponding
    /// prototype and returns that (i.e., a function with no body). The code
    /// generator will later add the implementation (and potentially adapt cc
    /// and linkage further) when its gets to the function's code.
    ///
    /// func: The function.
    ///
    /// force_new: If true and a function of the corresponding name already
    /// exists, we create a new function with an adapted name. If false, the
    /// existing function will be returned.
    llvm::Function* llvmFunction(shared_ptr<Function> func, bool force_new = false);

    /// Returns the LLVM function for a given ID. A function with that name
    /// must exist in the compiled HILTI module. This method handles
    /// not-yet-generated functions just as llvmFunction(shared_ptr<Function>)
    /// does.
    ///
    /// id: The name of the function.
    ///
    /// Returns: The LLVM function.
    llvm::Function* llvmFunction(shared_ptr<ID> id);

    /// Returns the LLVM function for a given name A function with that name
    /// must exist in the compiled HILTI module. This method handles
    /// not-yet-generated functions just as llvmFunction(shared_ptr<Function>)
    /// does.
    ///
    /// name: The name of the function.
    ///
    /// Returns: The LLVM function.
    llvm::Function* llvmFunction(const string& name);

    /// Returns the LLVM function that corresponds to entry function
    /// triggering execution of the hook. That function will actually
    /// generated by the linker, but calling this methods puts everythign in
    /// place that's necessary for the linker to do its job.
    ///
    /// hook: The hook.
    ///
    /// Returns: The function with the corresponding signature.
    llvm::Function* llvmFunctionHookRun(shared_ptr<Hook> hook);

    /// Returns the LLVM value for a HILTI expression.
    ///
    /// This method branches out the Loader to do its work.
    ///
    /// expr: The expression to evaluate.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    ///
    /// cctor: XXX
    ///
    /// Returns: The computed LLVM value.
    llvm::Value* llvmValue(shared_ptr<Expression> expr, shared_ptr<hilti::Type> coerce_to = nullptr,
                           bool cctor = false);

    /// Stores the LLVM value for a HILTI expression into an address.
    ///
    /// This method branches out the Loader to do its work.
    ///
    /// dst: The address where to store the value.
    ///
    /// expr: The expression to evaluate.
    ///
    /// coerce_to: If given, the expr is first coerced into this type before
    /// it's evaluated. It's assumed that the coercion is legal and supported.
    ///
    /// cctor: XXX
    void llvmValueInto(llvm::Value* dst, shared_ptr<Expression> expr,
                       shared_ptr<hilti::Type> coerce_to = nullptr, bool cctor = false);

    /// Returns the address of a value referenced by an expression, if
    /// possible. For types that don't support taking their address, returns
    /// null.
    ///
    /// expr: The expression to evaluate.
    llvm::Value* llvmValueAddress(shared_ptr<Expression> expr);

    /// Returns the LLVM value for the current execution context. Depending on
    /// what object currently being generated, the value may refer to
    /// different things.
    ///
    /// Returns: The LLVM value.
    llvm::Value* llvmExecutionContext();

    /// Returns the LLVM value for global thread manager.
    ///
    /// Returns: The LLVM value.
    llvm::Value* llvmThreadMgr();

    /// Returns the LLVM value for the global execution context.
    ///
    /// Returns: The LLVM value.
    llvm::Value* llvmGlobalExecutionContext();

    /// Returns the size of an LLVM constant. This is a helper function
    /// implementing a \c sizeof operator, which LLVM does not provide out of
    /// the box.
    llvm::Constant* llvmSizeOf(llvm::Constant* v);

    /// Returns the size of an LLVM type. This is a helper function
    /// implementing a \c sizeof operator, which LLVM does not provide out
    /// of the box.
    llvm::Constant* llvmSizeOf(llvm::Type* t);

    /// Returns the size of an LLVM type for the current target platform. Note
    /// that the returned value is non-portable, if possible llvmSizeOf()
    /// should be prefered.
    uint64_t llvmSizeOfForTarget(llvm::Type* t);

    /// Stores an LLVM value at the location associated with a HILTI
    /// expression.
    ///
    /// This method branches out to the Storer to do its work.
    ///
    /// target: The expression to evaluate to get the location where to store
    /// the value.
    ///
    /// value: The value to store a +0.
    void llvmStore(shared_ptr<hilti::Expression> target, llvm::Value* value,
                   bool dtor_first = true);

    /// Stores an LLVM value at the location associated with a HILTI
    /// instruction's target operand. expression.
    ///
    /// This methods branches out to the Storer to do its work.
    ///
    /// instr: The instruction's which's target operand gives us the
    /// destination for the store operation.
    ///
    /// value: The value to store at +0.
    void llvmStore(statement::Instruction* instr, llvm::Value* value, bool dtor_first = true);

    /// Returns: A pair in which the first element is the unpacked value and
    /// the second is an iterator pointing just beyond the last consumed byte.
    /// Neither tuple elements will have their cctor applied.
    std::pair<llvm::Value*, llvm::Value*> llvmUnpack(shared_ptr<Type> type, llvm::Value* begin,
                                                     llvm::Value* end, llvm::Value* fmt,
                                                     llvm::Value* arg = nullptr,
                                                     shared_ptr<Type> arg_type = nullptr,
                                                     const Location& location = Location::None);

    /// Unpacks an instance of a type from binary data (like in the \c unpack
    /// instruction). Exception semantics are as with \c unpack.
    ///
    /// type: The type to unpack an instance of.
    ///
    /// begin: A byte iterator marking the first input byte.
    ///
    /// end: A byte iterator marking the position one beyond the last
    /// consumable input byte. *end* may be null to indicate unpacking until
    /// the end of the bytes object is encountered.
    ///
    /// fmt: Specifies the binary format of the input bytes as one of the
    /// ``Hilti::Packed`` labels.
    ///
    /// arg: Additional format-specific parameter, required by some formats;
    /// nullptr if not.
    ///
    /// l: A location associagted with the unpack operaton.
    ///
    /// Returns: A pair in which the first element is the unpacked value and
    /// the second is an iterator pointing just beyond the last consumed byte.
    /// Neither tuple element will have its cctor applied.
    std::pair<llvm::Value*, llvm::Value*> llvmUnpack(shared_ptr<Type> type,
                                                     shared_ptr<Expression> begin,
                                                     shared_ptr<Expression> end,
                                                     shared_ptr<Expression> fmt,
                                                     shared_ptr<Expression> arg = nullptr,
                                                     const Location& location = Location::None);

    /// Packs an instance of a type into binary data (like in the \c pack
    /// instruction).
    ///
    /// value: The value to pack.
    ///
    /// type: The type of *value*.
    ///
    /// fmt: Specifies the binary format of the input bytes as one of the
    /// ``Hilti::Packed`` labels.
    ///
    /// arg: Additional format-specific parameter, required by some formats;
    /// nullptr if not.
    ///
    /// arg_type: Type of additional format-specific parameter; nullptr if not
    /// used.
    ///
    /// l: A location associagted with the unpack operaton.
    ///
    /// Returns: The packed value as a bytes instance.
    llvm::Value* llvmPack(llvm::Value* value, shared_ptr<Type> type, llvm::Value* fmt,
                          llvm::Value* arg = nullptr, shared_ptr<Type> arg_type = nullptr,
                          const Location& location = Location::None);

    /// Packs an instance of a type into binary data (like in the \c pack
    /// instruction).
    ///
    /// value: The value to pack.
    ///
    /// type: The type of *value*.
    ///
    /// fmt: Specifies the binary format of the input bytes as one of the
    /// ``Hilti::Packed`` labels.
    ///
    /// arg: Additional format-specific parameter, required by some formats;
    /// nullptr if not.
    ///
    /// l: A location associagted with the unpack operaton.
    ///
    /// Returns: The packed value as a bytes instance.
    llvm::Value* llvmPack(shared_ptr<Expression> value, shared_ptr<Expression> fmt,
                          shared_ptr<Expression> arg = nullptr,
                          const Location& location = Location::None);

    /// Returns a global's index in the module-wide array of globals. Each
    /// module keeps an array with all its globals as part of HILTI's
    /// execution context. This methods returns the index into that array for
    /// a specific variable.
    ///
    /// var: The variable for which to return the index.
    ///
    /// Returns: The LLVM value representing the index, suitable to use with a
    /// GEP instruction.
    llvm::Value* llvmGlobalIndex(Variable* var);

    /// Returns a global's index in the module-wide array of globals. Each
    /// module keeps an array with all its globals as part of HILTI's
    /// execution context. This methods returns the index into that array for
    /// a specific variable.
    ///
    /// var: The variable for which to return the index.
    ///
    /// Returns: The LLVM value representing the index, suitable to use with a
    /// GEP instruction.
    llvm::Value* llvmGlobalIndex(shared_ptr<Variable> var)
    {
        return llvmGlobalIndex(var.get());
    }

    /// Coerces an LLVM value from one type into another. It assumes that the
    /// coercion is legal and supported. If not, results are undefined. The
    /// returned value has its ccotr appied, and the source value its dtor.
    ///
    /// The method uses the Coercer to do its work.
    ///
    /// value: The value to coerce.
    ///
    /// src: The source type.
    ///
    /// dst: The destination type.
    ///
    /// cctor: True if the result should have its cctor applied.
    ///
    /// Returns: The coerced value.
    llvm::Value* llvmCoerceTo(llvm::Value* value, shared_ptr<hilti::Type> src,
                              shared_ptr<hilti::Type> dst, bool cctor);

    /// Returns an LLVM type from \c libhilti.ll. The method looks up a name
    /// in the library and returns the corresponding type. It's an error if
    /// the name doesn't define a type.
    ///
    /// name: The name to lookup.
    ///
    /// Returns: The LLVM type.
    llvm::Type* llvmLibType(const string& name);

    /// Returns an LLVM function from \c libbhilti.ll. The method looks up a
    /// name in the library and returns the corresponding function. It's an
    /// error if the name doesn't define a function.
    ///
    /// name: The name to lookup.
    ///
    /// Returns: The LLVM type.
    llvm::Function* llvmLibFunction(const string& name);

    /// Returns an LLVM global from \c libbhilti.ll. The method looks up a
    /// name in the library and returns the corresponding value. It's an error
    /// if the name doesn't define a global.
    ///
    /// name: The name to lookup.
    ///
    /// Returns: The LLVM global (which, as usual with LLVM, is a pointer to
    /// the actual value).
    llvm::GlobalVariable* llvmLibGlobal(const string& name);

    /// Returns the type information structure for a type.
    ///
    /// This methods uses the TypeBuilder to do its work.
    ///
    /// type: The type to return the information for.
    ///
    /// Returns: The object with the type's information.
    TypeInfo* typeInfo(shared_ptr<hilti::Type> type);

    /// Returns the LLVM type that corresponds to a HILTI type.
    ///
    /// This methods uses the TypeBuilder to do its work.
    ///
    /// type: The type to return the LLVM type for.
    ///
    /// Returns: The corresponding LLVM type.
    llvm::Type* llvmType(shared_ptr<hilti::Type> type);

    /// Returns the LLVM type for a function. This includes doing all the ABI
    /// alignments.
    ///
    /// ftype: The function type.
    ///
    /// Returns: The final LLVM type.
    llvm::FunctionType* llvmFunctionType(shared_ptr<type::Function> ftype);

    /// Returns the LLVM calling convention corresponding to a HILTI one.
    ///
    /// cc: The HILTI convention.
    ///
    /// Returns: The LLVM calling convention.
    llvm::CallingConv::ID llvmCallingConvention(type::function::CallingConvention cc);

    /// Returns the LLVM initialization value for a HILTI type.
    ///
    /// This methods uses the TypeBuilder to do its work.
    ///
    /// type: The type to return the initalization value for.
    ///
    /// Returns: The LLVM initialization value at +0.
    llvm::Constant* llvmInitVal(shared_ptr<hilti::Type> type);

    /// Returns a pointer to the LLVM RTTI object for a HILTI type.
    ///
    /// This methods uses the TypeBuilder to do its work.
    ///
    /// type: The type to return the RTTI for.
    ///
    /// Returns: The corresponding LLVM value.
    llvm::Constant* llvmRtti(shared_ptr<hilti::Type> type);

#if 0
   /// Returns a pointer to a pointer to the LLVM RTTI object for a HILTI
   /// type.
   ///
   /// This methods uses the TypeBuilder to do its work.
   ///
   /// type: The type to return the RTTI for.
   ///
   /// Returns: The corresponding LLVM value.
   llvm::GlobalVariable* llvmRttiPtr(shared_ptr<hilti::Type> type);
#endif

    /// Returns the LLVM type the code generator uses for void function
    /// results.
    llvm::Type* llvmTypeVoid();

    /// Returns the LLVM type the code generator uses for ``hlt_string``
    /// objects. This is a shortcut to calling llvmType() for a type::String
    /// object.
    llvm::Type* llvmTypeString();

    /// Returns an LLVM integer type of given bit width.
    ///
    /// width: The bit width.
    llvm::Type* llvmTypeInt(int width);

    /// Returns an LLVM double type.
    llvm::Type* llvmTypeDouble();

    /// Returns an LLVM float type.
    llvm::Type* llvmTypeFloat();

    typedef std::vector<llvm::Type*> type_list;

    /// Returns an LLVM struct type built from given field types.
    ///
    /// name: A name to associate with the name. If given, the methods creates
    /// an "identified" type; if not, a "literal" type. See
    /// http://blog.llvm.org/2011/11/llvm-30-type-system-rewrite.html.
    ///
    /// fields: The fields' types.
    ///
    /// packed: True to creat a packed struct.
    llvm::Type* llvmTypeStruct(const string& name, const type_list& fields,
                               bool packed = false); // name="" create anon struct.

    /// Returns an LLVM pointer type to another type.
    ///
    /// t: The type the pointer is to point to. If null, \c i8* is return (aka
    /// \c void*).
    llvm::Type* llvmTypePtr(llvm::Type* t = 0); // 0 yields void*.

    /// Returns the LLVM type of a HILTI execution context.
    llvm::Type* llvmTypeExecutionContext();

    /// Returns the LLVM type for a pointer to a HILTI exception.
    llvm::Type* llvmTypeExceptionPtr();

    /// Returns the LLVM type for a HILTI exception type.
    ///
    /// excpt: The exception type.
    llvm::Constant* llvmExceptionTypeObject(shared_ptr<type::Exception> excpt);

    /// Returns the LLVM type of a HILTI type information object.
    llvm::Type* llvmTypeRtti();

    /// Returns an LLVM integer constant of given value.
    ///
    /// val: The value.
    ///
    /// width: The bit width.
    llvm::ConstantInt* llvmConstInt(int64_t val, int64_t width);

    /// Returns an LLVM double constant of given value.
    ///
    /// val: The value.
    llvm::Constant* llvmConstDouble(double val);

    /// Returns an LLVM null value for a given type.
    ///
    /// t: The type which's null value to return. If null, uses \c i8* (aka \c
    /// void*).
    llvm::Constant* llvmConstNull(llvm::Type* t = 0);

    /// Returns a \c iterator<bytes> referencing the end position of (any) \c byts object.
    llvm::Constant* llvmConstBytesEnd();

    /// Returns the value of an enum label.
    ///
    /// label: The qualified name to look up.
    llvm::Value* llvmEnum(const string& label);

    typedef std::vector<llvm::Constant*> constant_list;

    /// Returns an LLVM struct constant with fields initialized. There's also
    /// a version of this method that works with values rather than constants,
    /// llvmValueStruct().
    ///
    /// elems: The field values.
    ///
    /// packed: True if the constant is to represent a packed struct type.
    llvm::Constant* llvmConstStruct(const constant_list& elems, bool packed = false);

    /// Returns a named LLVM struct constant with fields initialized.
    ///
    /// type: The struct will be created as of this type (which must
    /// be a struct type obviousl).
    ///
    /// elems: The field values.
    ///
    llvm::Constant* llvmConstStruct(llvm::Type* type, const constant_list& elems);

    /// Returns an LLVM array constant with members initialized.
    ///
    /// t: The type of the array's elements.
    ///
    /// elems: The list of elements to initialize the array with. Can be empty to create an empty
    /// array.
    llvm::Constant* llvmConstArray(llvm::Type* t, const std::vector<llvm::Constant*>& elems);

    /// Returns an LLVM array constant with members initialized.
    ///
    /// elems: The list of elements to initialize the array with. Must not be
    /// empty, for that use llvmConstArray(llvm::Type*, const
    /// std::vector<llvm::Constant*>&) instead.
    llvm::Constant* llvmConstArray(const std::vector<llvm::Constant*>& elems);

    /// Returns an \c i8 array initialized with characters from a string and
    /// terminated with a null byte.
    ///
    /// str: The characters to initialize the array with.
    llvm::Constant* llvmConstAsciiz(const string& str);

    /// Returns a pointer to an \c i8 array initialized with characters from a
    /// string and terminated with a null byte.
    ///
    /// str: The characters to initialize the array with.
    llvm::Constant* llvmConstAsciizPtr(const string& str);

    /// Cast an LLVM constant into a different type. This is just a shortcut
    /// for using LLVM's ConstantExpr.
    ///
    /// c: The constant.
    ///
    /// t: The target type.
    ///
    /// Returns: The casted constant.
    llvm::Constant* llvmCastConst(llvm::Constant* c, llvm::Type* t);

    // Reinterprets an existing value as representing a different type.
    // Returns the value with that new type.
    //
    // val: The value to reinterpret.
    //
    // ntype: The type to interpret the value as.
    llvm::Value* llvmReinterpret(llvm::Value* val, llvm::Type* ntype);

    /// Returns a constant representing a GetElementPtr index. This is just a
    /// shortcut to creating the constant "manually.
    ///
    /// idx: The index to initialize the constant with.
    llvm::Constant* llvmGEPIdx(int64_t idx);

    /// Returns the LLVM value corresponding to a HILTI string of given value.
    ///
    /// s: The string value.
    ///
    /// Returns: The value at +0.
    llvm::Value* llvmString(const string& s);

    /// Returns a pointer to the LLVM value corresponding to a HILTI string of
    /// given value.
    ///
    /// s: The string value.
    ///
    /// Returns: A pointer to the value at +0.
    llvm::Value* llvmStringPtr(const string& s);

    /// Returns an LLVM struct value with fields initialized. There's also a
    /// version of this method that works with constants rather than arbitrary
    /// values, llvmConstStruct().
    ///
    /// type: The struct will be created as of this type (which must
    /// be a struct type obviousl).
    ///
    /// elems: The field values.
    ///
    /// packed: True if the constant is to represent a packed struct type.
    llvm::Value* llvmValueStruct(llvm::Type* type, const std::vector<llvm::Value*>& elems,
                                 bool packed = false);

    /// Returns an LLVM struct value with fields initialized. There's also a
    /// version of this method that works with constants rather than arbitrary
    /// values, llvmConstStruct().
    ///
    /// elems: The field values.
    ///
    /// packed: True if the constant is to represent a packed struct type.
    llvm::Value* llvmValueStruct(const std::vector<llvm::Value*>& elems, bool packed = false);

    /// Adds a new global constant to the LLVM module. The constant will have
    /// private linkage (but that can be changed afterwards).
    ///
    /// name: A name to use for the constant. Note that by default this is
    /// only a prefix, and the name will be further mangled.
    ///
    /// val: The value to initialize the constant with.
    ///
    /// use_name_as_is: If true, \c name will be used literally without
    /// further mangling.
    ///
    /// Returns: The new global.
    llvm::GlobalVariable* llvmAddConst(const string& name, llvm::Constant* val,
                                       bool use_name_as_is = false);

    /// Adds a new global value to the LLVM module. The global will have
    /// private linkage (but that can be changed afterwards).
    ///
    /// name: A name to use for the global. Note that by default this is only
    /// a prefix, and the name will be further mangled.
    ///
    /// type: The type of the global.
    ///
    /// init_val: An optional value to initialize the global with.
    ///
    /// use_name_as_is: If true, \c name will be used literally without
    /// further mangling.
    ///
    /// Returns: The new global.
    llvm::GlobalVariable* llvmAddGlobal(const string& name, llvm::Type* type,
                                        llvm::Constant* init = 0, bool use_name_as_is = false);

    /// Adds a new global value to the LLVM module. The global will have
    /// private linkage (but that can be changed afterwards).
    ///
    /// name: A name to use for the global. Note that by default this is only
    /// a prefix, and the name will be further mangled.
    ///
    /// init: A to initialize the global with.
    ///
    /// use_name_as_is: If true, \c name will be used literally without
    /// further mangling.
    ///
    /// Returns: The new global.
    llvm::GlobalVariable* llvmAddGlobal(const string& name, llvm::Constant* init,
                                        bool use_name_as_is = false);

    /// Adds a new local variable to the current LLVM function. Locals created
    /// with this method should correspond to variables defined by the user at
    /// the HILTI level. To create internal temporaries for use by the code
    /// generator, use llvmAddTmp() instead.
    ///
    /// name: A name to use for the local. Note that it's not necessarily used
    /// literally and may be adpated further.
    ///
    /// type: The type of the local.
    ///
    /// init: An optional intialization value. If this is given, the variable
    /// will be initialized with it in the current builder() (but not earlier,
    /// where it's undefined). If not given, the variable will be initialized
    /// with the type's default value right at the beginning of the current
    /// function.
    ///
    /// Returns: The new local variable.
    llvm::Value* llvmAddLocal(const string& name, shared_ptr<Type> type,
                              shared_ptr<Expression> init = 0, bool hoisted = false);

    /// Adds a new local temporary variable to the current LLVM function.
    /// Temporaries don't correspond to a user-defined variable but are
    /// created internally by the code generator to store intermediaries. They
    /// are also not kept alive automatically across safepoints, you need to
    /// cctor them as appropiate.
    ///
    /// name: A name to use for the temporary. Note that it's not necessarily
    /// used literally and may be adpated further.
    ///
    /// type: The type of the temporary.
    ///
    /// init: An optional intialization value.
    ///
    /// reuse: If in the same function a temporary has already been created
    /// with the same name, that one will be returned instead of creating a
    /// new one. Note that no type-check is done.
    ///
    /// Returns: The new temporary variable.
    llvm::Value* llvmAddTmp(const string& name, llvm::Type* type, llvm::Value* init = 0,
                            bool reuse = false, int alignment = 0);

    /// Adds a new local temporary variable to the current LLVM function.
    /// Temporaries don't correspond to a user-defined variable but are
    /// created internally by the code generator to store intermediaries.
    ///
    /// name: A name to use for the temporary. Note that it's not necessarily
    /// used literally and may be adpated further.
    ///
    /// init: An intialization value.
    ///
    /// reuse: If in the same function a temporary has already been created
    /// with the same name, that one will be returned instead of creating a
    /// new one. Note that no type-check is done.
    ///
    /// Returns: The new temporary variable.
    llvm::Value* llvmAddTmp(const string& name, llvm::Value* init = 0, bool reuse = false);

    typedef std::pair<string, shared_ptr<hilti::Type>> parameter;
    typedef std::vector<parameter> parameter_list;

    typedef std::pair<string, llvm::Type*> llvm_parameter;
    typedef std::vector<llvm_parameter> llvm_parameter_list;

    /// Adds a new LLVM function to the current module.
    ///
    /// func: The HILTI function corresponding to the function being created.
    ///
    /// internal: True if the function is not to be visible across LLVM module
    /// boundaries. This will set the linkage to internal and mangle the name
    /// appropiately.
    ///
    /// cc: The calling convention the created function is to use. The default
    /// is to use whatever the function's type specifies, but setting \cc
    /// differently overrides that.
    ///
    /// force_name: If given, the function will have this name instead of the
    /// one normally derived from *func* implicitly.
    ///
    /// skip_ctx: For cc HILTI-C, do not add the context parameter.
    ///
    /// Returns: The new function.
    llvm::Function* llvmAddFunction(shared_ptr<Function> func, bool internal,
                                    type::function::CallingConvention cc = type::function::DEFAULT,
                                    const string& force_name = "", bool skip_ctx = false);

    /// Adds a new LLVM function to the current module.
    ///
    /// name: The name of the function. Note that the name will be further mangled.
    ///
    /// rytpe: The return type of the function.
    ///
    /// params: The parameters to the function.
    ///
    /// internal: True if the function is not to be visible across LLVM module
    /// boundaries. This will set the linkage to internal and mangle the name
    /// appropiately.
    ///
    /// cc: The calling convention the created function is to use.
    ///
    /// skip_ctx: For cc HILTI-C, do not add the context parameter.
    ///
    /// Returns: The new function.
    llvm::Function* llvmAddFunction(const string& name, llvm::Type* rtype, parameter_list params,
                                    bool internal, type::function::CallingConvention cc,
                                    bool skip_ctx = false);

    /// XXX.
    llvm::Function* llvmAddFunction(const string& name, llvm::Type* rtype,
                                    llvm_parameter_list params, bool internal,
                                    bool force_name = false);

    typedef std::vector<shared_ptr<Expression>> expr_list;
    typedef std::function<void(CodeGen* cg)> call_exception_callback_t;

    /// Generates an LLVM call to a HILTI function. This method handles all
    /// calling conventions.
    ///
    /// func: The HILTI function to call.
    ///
    /// ftype: The type of the callee.
    ///
    /// args: The parameters to evaluate and pass to the callee.
    ///
    /// cctor_result: If true, the return value will have its cctor function
    /// applied; if false, it will be returned at +0.
    ///
    /// excpt_check: If false, the call will not be followed for a check
    /// whether the callee has raised an exception. Use that if you're sure
    /// that it won't, or if for some other reason you know it's not an issue.
    ///
    /// Returns: The call instruction created.
    llvm::Value* llvmCall(shared_ptr<Function> func, const expr_list args,
                          bool cctor_result = false, bool excpt_check = true,
                          call_exception_callback_t excpt_callback = [](CodeGen* cg) {});

    /// Generates an LLVM call to a HILTI function. This method handles all
    /// calling conventions.
    ///
    /// llvm_func: The LLVM function corresponding to the HILTI function we
    /// want to call (usually retrieved via llvmFunction()).
    ///
    /// ftype: The type of the callee.
    ///
    /// args: The parameters to evaluate and pass to the callee.
    ///
    /// cctor_result: If true, the return value will have its cctor function
    /// applied; if false, it will be returned at +0.
    ///
    /// excpt_check: If false, the call will not be followed for a check
    /// whether the callee has raised an exception. Use that if you're sure
    /// that it won't, or if for some other reason you know it's not an issue.
    ///
    /// Returns: The call instruction created.
    llvm::Value* llvmCall(llvm::Value* llvm_func, shared_ptr<type::Function> ftype,
                          const expr_list args, bool cctor_result = false, bool excpt_check = true,
                          call_exception_callback_t excpt_callback = [](CodeGen* cg) {});

    /// Generates an LLVM call to a HILTI function. This method handles all
    /// calling conventions.
    ///
    /// name: The name of the HILTI function to call, which must be resolvable
    /// in the current module's scope.
    ///
    /// ftype: The type of the callee.
    ///
    /// args: The parameters to evaluate and pass to the callee.
    ///
    /// cctor_result: If true, the return value will have its cctor function
    /// applied; if false, it will be returned at +0.
    ///
    /// excpt_check: If false, the call will not be followed for a check
    /// whether the callee has raised an exception. Use that if you're sure
    /// that it won't, or if for some other reason you know it's not an issue.
    ///
    /// Returns: The call instruction created.
    llvm::Value* llvmCall(const string& name, const expr_list args, bool cctor_result = false,
                          bool excpt_check = true,
                          call_exception_callback_t excpt_callback = [](CodeGen* cg) {});

    typedef std::vector<llvm::Value*> value_list;

    /// Generates a straight LLVM call to a C function, without adapting
    /// parameters to HILTI'c calling conventions.
    ///
    /// llvm_func: The function to call.
    ///
    /// args: The parameters to the call.
    ///
    /// add_hiltic_args: If true, the method does add the standard additional
    /// \c C-HILTI arguments to the call.
    ///
    /// excpt_check: If false, the call will not be followed for a check
    /// whether the callee has raised an exception. Use that if you're sure
    /// that it won't, or if for some other reason you know it's not an issue.
    ///
    /// Returns: The call instruction created. The result value will be passed
    /// straight through, so cctor semantics depend on the callee.
    llvm::CallInst* llvmCallC(llvm::Value* llvm_func, const value_list& args, bool add_hiltic_args,
                              bool excpt_check = true);

    /// Generates a straight LLVM call to a C function defined in \c
    /// libhilti.ll, without adapting parameters to HILTI'c calling
    /// conventions.
    ///
    /// llvm_func: The function to call.
    ///
    /// args: The parameters to the call.
    ///
    /// add_hiltic_args: If true, the method does add the standard additional
    /// \c C-HILTI arguments to the call.
    ///
    /// excpt_check: If false, the call will not be followed for a check
    /// whether the callee has raised an exception. Use that if you're sure
    /// that it won't, or if for some other reason you know it's not an issue.
    ///
    ///
    /// Returns: The call instruction created. The result value will be passed
    /// straight through, so cctor semantics depend on the callee.
    llvm::CallInst* llvmCallC(const string& llvm_func, const value_list& args, bool add_hiltic_args,
                              bool excpt_check = true);

    /// Calls an LLVM intrinsic.
    ///
    /// intr: The intrinsic's ``INTR_*`` enum constant. See \c
    /// include/llvm/Intrinsics.gen for list
    ///
    /// ty: The types determining the version to pick for overloaded intrinsics; empty if not
    /// needed.
    ///
    /// args: The arguments to call the intrinsic with.
    ///
    /// Returns: Whatever the intrinsic returns.
    llvm::CallInst* llvmCallIntrinsic(llvm::Intrinsic::ID intr, std::vector<llvm::Type*> tys,
                                      const value_list& args);

    /// Generates an llvm.expect intrinsic.
    ///
    /// v: The actual value.
    ///
    /// e: The expected value.
    llvm::Value* llvmExpect(llvm::Value* v, llvm::Value* e);

    /// Generates the code for a HILTI \c return or \c return.void statement.
    /// This methods must be used instead of a plain LLVM \c return
    /// instruction; it makes sure to run any necessary end-of-function
    /// cleanup code.
    ///
    /// rtype: The type of the result. For void, either type::Void or null can
    /// be used.
    ///
    /// result: The result to return from the current function, or null if the
    /// return type is void. This comes in at +0.
    ///
    /// result_cctored: If true, the result is already cctored.
    void llvmReturn(shared_ptr<Type> rtype = 0, llvm::Value* result = 0,
                    bool result_cctored = false);

    /// Creates a new callable insance and binds a call to it. Arguments are
    /// the same as with the corresponding llvmCall() method.
    ///
    /// ref: True for the callable returning values at +1, false for +0. In
    /// the latter case, the result will have been added to the context's
    /// nullbuffer so that at the next safepoint it will be deleted if not
    /// further refs take place.
    ///
    /// cctor_callable: True if the callable should be returned at +1.
    ///
    /// Returns: The callable, which can be executed with llvmCallableRun().
    /// It will be returned at +0 or + 1, depending on cctor_callable.
    llvm::Value* llvmCallableBind(shared_ptr<Function> func, shared_ptr<type::Function> ftype,
                                  const expr_list args, bool ref, bool excpt_check = true,
                                  bool deep_copy_args = false, bool cctor_callable = false);

    /// Creates a new callable insance and binds a call to it. Arguments are
    /// the same as with the corresponding llvmCall() method.
    ///
    /// ref: True for the callable returning values at +1, false for +0. In
    /// the latter case, the result will have been added to the context's
    /// nullbuffer so that at the next safepoint it will be deleted if not
    /// further refs take place.
    ///
    /// cctor_callable: True if the callable should be returned at +1.
    ///
    /// Returns: The callable, which can be executed with llvmCallableRun().
    /// It will be returned at +0 or + 1, depending on cctor_callable.
    llvm::Value* llvmCallableBind(llvm::Value* llvm_func, shared_ptr<type::Function> ftype,
                                  const expr_list args, bool ref, bool excpt_check = true,
                                  bool deep_copy_args = false, bool cctor_callable = false);

    /// Creates a new callable insance and binds the execution of a hook to
    /// it.
    ///
    /// ref: True for the callable returning values at +1, false for +0. In
    /// the latter case, the result will have been added to the context's
    /// nullbuffer so that at the next safepoint it will be deleted if not
    /// further refs take place.
    ///
    /// cctor_callable: True if the callable should be returned at +1.
    ///
    /// Returns: The callable, which can be executed with llvmCallableRun().
    /// It will be returned at +0 or + 1, depending on cctor_callable.
    llvm::Value* llvmCallableBind(shared_ptr<Hook> hook, const expr_list args, bool ref,
                                  bool excpt_check = true, bool deep_copy_args = false,
                                  bool cctor_callable = false);

    /// Executes a previously bound callable.
    ///
    /// cty: The type of the callable.
    ///
    /// c: A pointer to the callable.
    ///
    /// args: The (non-bound) arguments to the callable.
    ///
    /// Returns: The return value for callables that have, or null for ones
    /// without. It will be returned at +0 or +1 depending on how that
    /// callable was bound.
    llvm::Value* llvmCallableRun(shared_ptr<type::Callable> cty, llvm::Value* callable,
                                 const expr_list args);

    /// Calls a HILTI function inside a newly created fiber. If the fiber
    /// throws an exception, that will be set in the current context,
    /// including a YieldException if the fiber has yielded. However, we don't
    /// perform any excpeption handling, the caller needs to do that if
    /// desired.
    ///
    /// The arguments are the same as with the corresponding llvmCall()
    /// method.
    ///
    /// Returns: If the functin returns a value, the value. Otherwise, null.
    llvm::Value* llvmCallInNewFiber(shared_ptr<Function> func, const expr_list args,
                                    bool cctor_result);

    /// Starts, or resumes, a fiber. If the fiber throws an exception, that
    /// will be set in the current context, including a YieldException if the
    /// fiber has yielded. However, we don't perform any excpeption handling,
    /// the caller needs to do that if desired.
    ///
    /// fiber: The fiber. If not run before, its processing will kick off. If
    /// it has yielded, it will resume where left off.
    ///
    /// rtype: The type of the fiber's result if it returns one; null if not.
    ///
    /// Returns: If the fiber returns a value, the value. Otherwise, null.
    /// Cctor semantics are determined by the fiber's code) (see
    /// llvmCallInNewFiber().
    llvm::Value* llvmFiberStart(llvm::Value* fiber, shared_ptr<Type> rtype);

    /// Yields from inside a fiber. Must be used only when we indeed are
    /// inside a fiber. When processing is resumed, execution wil continue
    /// right after the code generaged by this method. Optionally, one can
    /// give the scheduler a resource to wait for becoming available before
    /// returnin control.
    ///
    /// fiber: The fiber we are currently running inside.
    ///
    /// blockable_ty: The type fo the blockable resource to wait for; this
    /// must be a type of trait type::trait::Blockable. If given, \a
    /// blockable_val must be so as well.
    ///
    /// blockable_val: The value for the blockable resources, i.e., an
    /// instance of type \a blockable_ty.
    void llvmFiberYield(llvm::Value* fiber, shared_ptr<Type> blockable_ty = nullptr,
                        llvm::Value* blockable_val = nullptr);

    /// Triggers execution of a HILTI hook.
    ///
    /// hook: The hook.
    ///
    /// args: The parameters to evaluate and pass to the hook.
    ///
    /// result: If the hook as a void result type, this must be null. If not,
    /// it must be \a pointer to an instance of the corresponding LLVM type.
    /// If one of the hook functions returns a value via \c hook.stop, that
    /// will be stored there. If none does, the current value will be left
    /// untouched.
    ///
    /// cctor_result: If true, the result will have its cctor function
    /// applied; if false, it will be returned at +0.
    ///
    /// Returns: A boolean \c i1 LLVM value indicating whether any hook
    /// function has executed \c hook.stop (true) or not (false).
    llvm::Value* llvmRunHook(shared_ptr<Hook> hook, const expr_list& args, llvm::Value* result,
                             bool cctor_result);

    /// Adds meta data for a hook implementation that the linker will
    /// evaluate. This is primarily meant for being called from the statement
    /// builder.
    ///
    /// hook: The hook.
    ///
    /// llvm_func: The LLVM function corresponding to the hook implementation.
    void llvmAddHookMetaData(shared_ptr<Hook> hook, llvm::Value* llvm_func);

    /// Inserts code that checks with a \c C-HILTI function has raised an
    /// exception. If so, the code will reraise that as a HILTI exception.
    ///
    /// excpt: The value where the \c C-HILTI function would have stored its
    /// exception (i.e., usually the \c excpt parameter).
    void llvmCheckCException(llvm::Value* excpt, bool raise = true);

    /// Creates a new exception instance.
    ///
    /// exception: The name of the exception's type. The name must define an
    /// exception type in \c libhilti.ll.
    ///
    /// node: A node to associate with the exception as its source. We use the
    /// node location information.
    ///
    /// arg: The exception's argument if the type takes one, or null if not.
    ///
    /// Returns: The exception instance at +0.
    llvm::Value* llvmExceptionNew(const string& exception, const Location& l, llvm::Value* arg);

    /// Returns the exception's argument as a void pointer.
    ///
    /// excpt: A pointer to the exception to retrieve the argument from.
    llvm::Value* llvmExceptionArgument(llvm::Value* excpt);

    /// Returns the exception's fiber if resumable, or an LLVM null value if not.
    ///
    /// excpt: A pointer to the exception to retrieve the fiber from.
    llvm::Value* llvmExceptionFiber(llvm::Value* excpt);

    /// Generates code to raise an exception. When executed, the code will
    /// *not* return control back to the current block.
    ///
    /// exception: The name of the exception's type. The name must define an
    /// exception type in \c libhilti.ll.
    ///
    /// node: A node to associate with the exception as its source. We use the
    /// node location information.
    ///
    /// arg: The exception's argument if the type takes one, or null if not.
    void llvmRaiseException(const string& exception, shared_ptr<Node> node,
                            llvm::Value* arg = nullptr);

    /// Generates code to raise an exception. When executed, the code will
    /// *not* return control back to the current block.
    ///
    /// exception: The name of the exception's type. The name must define an
    /// exception type in \c libhilti.ll.
    ///
    /// loc: Location information to associate with the exception.
    ///
    /// arg: The exception's argument if the type takes one, or null if not; at +0.
    void llvmRaiseException(const string& exception, const Location& l, llvm::Value* arg = nullptr);

    /// Generates code to raise an exception. When executed, the code will
    /// *not* return control back to the current block.
    ///
    /// excpt: The LLVM exception object to raise. The must have been cctored
    /// already.
    ///
    /// dtor: If true, the \a excpt's dtor will be called.
    void llvmRaiseException(llvm::Value* excpt, bool dtor = false);

    /// Rethrows an already thrown exception by unwinding the stack until a
    /// handler is found.
    void llvmRethrowException();

    /// Clears the current exception.
    void llvmClearException();

    /// Extracts the current exception from the execution context.
    llvm::Value* llvmCurrentException();

    /// Extracts the current fiber from the execution context.
    llvm::Value* llvmCurrentFiber();

    /// Extracts the current virtual thread ID from the execution context.
    llvm::Value* llvmCurrentVID();

    /// Returns the current thread context form the execution context. The
    /// returned value does not have its cctor applied yet.
    llvm::Value* llvmCurrentThreadContext();

    /// Sets the current thread context in the execution context.
    ///
    /// type: The type of the context, of type type::Context.
    ///
    /// ctx: The new value for the context. It's assumed to not have its cctor
    /// applied.
    void llvmSetCurrentThreadContext(shared_ptr<Type> type, llvm::Value* ctx);

#if 0 // Not used, runtime takes care of it.
   /// Sets the current fiber in the execution context.
   llvm::Value* llvmSetCurrentFiber(llvm::Value* fiber);

   /// Clears the current fiber in the execution context.
   llvm::Value* llvmClearCurrentFiber();
#endif

    /// Checks whether a connection has been raised and if so, triggers
    /// handling. Control flow may not return in that case if stack unwinding
    /// is needed.
    void llvmCheckException();

    /// Checks whether an exception is of a specific type.
    ///
    /// name: The qualified name of the excepetion (e.g., \c
    /// Hilti::WouldBlock).
    ///
    /// expt: The exception value. It's ok to pass null pointer, it will never
    /// match.
    ///
    /// Returns: A boolean value that's true if the the excpt is of the given type.
    llvm::Value* llvmMatchException(const string& name, llvm::Value* excpt);

    /// Checks whether an exception is of a specific type.
    ///
    /// etype: The exception type, of type type::Exception.
    ///
    /// expt: The exception value. It's ok to pass null pointer, it will never
    /// match.
    ///
    /// Returns: A boolean value that's true if the the excpt is of the given type.
    llvm::Value* llvmMatchException(shared_ptr<type::Exception> etype, llvm::Value* excpt);

    /// Wrapper method to create an LLVM \c call instruction that first checks
    /// the call's parameters for compatibility with the function's prototype
    /// If not matching, it dumps out debugging outout and abort execution.
    /// There's a separate version of this method for calls without parameters
    /// (though passing an empty array here works as well).
    ///
    /// callee: The function to call.
    ///
    /// args: The function parameters.
    ///
    /// name: The name LLVM will associate with the instruction.
    ///
    /// Returns: The created call instruction.
    llvm::CallInst* llvmCreateCall(llvm::Value* callee, llvm::ArrayRef<llvm::Value*> args,
                                   const llvm::Twine& name = "");

    /// Wrapper method to create an LLVM \c call instruction that first checks
    /// the call's parameters for compatibility with the function's prototype
    /// If not matching, it dumps out debugging outout and abort execution.
    /// This version of the method is for calls without parameters.
    ///
    /// callee: The function to call.
    ///
    /// name: The name LLVM will associate with the instruction.
    ///
    /// Returns: The created call instruction.
    llvm::CallInst* llvmCreateCall(llvm::Value* callee, const llvm::Twine& name = "");

    /// Wrapper method to create an LLVM \c sotre instructions that first
    /// checks operands for compatibility. If not matching, it dumps out
    /// debugging outout and abort execution.
    llvm::StoreInst* llvmCreateStore(llvm::Value* val, llvm::Value* ptr, bool isVolatile = false);

    /// Wrapper methods that creates an alloca instruction but preferable puts
    /// it into the starting block of the current function, where LLVM is more
    /// likely to optimize it away.
    llvm::AllocaInst* llvmCreateAlloca(llvm::Type* t, llvm::Value* array_size = 0,
                                       const llvm::Twine& name = "");

    /// Wrapper method to create an LLVM \c branch instruction that takes
    /// builders instead of blocks.
    llvm::BranchInst* llvmCreateBr(IRBuilder* builder);

    /// Wrapper method to create an LLVM \c conditional branch instruction
    /// that takes builders instead of blocks.
    llvm::BranchInst* llvmCreateCondBr(llvm::Value* cond, IRBuilder* true_, IRBuilder* false_);

    /// Wrapper method around LLVM's IsNull() that also works correctly for floating point values.
    llvm::Value* llvmCreateIsNull(llvm::Value* arg, const llvm::Twine& name = "");

    /// Wrapper method to create an LLVM \c gep instruction a bit more easily.
    /// There's also a const version of this method.
    ///
    /// addr: The base address to use.
    ///
    /// i1: The 1st index.
    ///
    /// i2: The 2st index, or null of none.
    ///
    /// i3: The 3st index, or null of none.
    ///
    /// i4: The 4st index, or null of none.
    ///
    /// Returns: The computed address.
    llvm::Value* llvmGEP(llvm::Value* addr, llvm::Value* i1, llvm::Value* i2 = 0,
                         llvm::Value* i3 = 0, llvm::Value* i4 = 0);

    /// Wrapper method to create an LLVM \c gep instruction a bit more easily.
    /// There's also a non-const version of this method.
    ///
    /// addr: The base address to use.
    ///
    /// i1: The 1st index.
    ///
    /// i2: The 2st index, or null of none.
    ///
    /// i3: The 3st index, or null of none.
    ///
    /// i4: The 4st index, or null of none.
    ///
    /// Returns: The computed address.
    llvm::Constant* llvmGEP(llvm::Constant* addr, llvm::Value* i1, llvm::Value* i2 = 0,
                            llvm::Value* i3 = 0, llvm::Value* i4 = 0);

    /// Wrapper method to create an LLVM \c insert_value instruction a bit
    /// more easily. There's also a const version, llvmConstInsertValue().
    ///
    /// aggr: The aggregate value to insert into.
    ///
    /// val: The value to insert.
    ///
    /// idx: The index to insert it at.
    ///
    /// Returns: The aggregate with the value inserted.
    llvm::Value* llvmInsertValue(llvm::Value* aggr, llvm::Value* val, unsigned int idx);

    /// Wrapper method to create an LLVM \c extract_value instruction a bit
    /// more easily. There's also a const version, llvmConstExtractValue().
    ///
    /// aggr: The aggregate value to extract from.
    ///
    /// idx: The index to extract.
    ///
    /// Returns: The extracted value.
    llvm::Value* llvmExtractValue(llvm::Value* aggr, unsigned int idx);

    /// Wrapper method to do an LLVM \c insert_value operation on a constant a
    /// bit more easily. There's also a non-const version, llvmInsertValue().
    ///
    /// aggr: The aggregate value to insert into.
    ///
    /// val: The value to insert.
    ///
    /// idx: The index to insert it at.
    ///
    /// Returns: The aggregate with the value inserted.
    llvm::Constant* llvmConstInsertValue(llvm::Constant* aggr, llvm::Constant* val,
                                         unsigned int idx);

    /// Wrapper method to do an LLVM \c extract_value operation on a constant
    /// a bit more easily. There's also a non-const version,
    /// llvmExtractValue().
    ///
    /// aggr: The aggregate value to extract from.
    ///
    /// idx: The index to extract.
    ///
    /// Returns: The extracted value.
    llvm::Constant* llvmConstExtractValue(llvm::Constant* aggr, unsigned int idx);

    /// Extract a range of bits from an integer value. All three arguments
    /// must be of same bitwidth.
    ///
    /// This mimics an old LLVM intrinsic which seems to have been removed (\c
    /// INTR_PART_SELECT).
    ///
    /// value: The integer value to extract bits from.
    ///
    /// low: The number of the range's low bit.
    ///
    /// high: The number of the range's high bit.
    ///
    /// Returns: The extracted bits, shifted so that the lowest one alings at
    /// bit zero. The result is undefined if the given range exceeds the bits
    /// available.
    llvm::Value* llvmExtractBits(llvm::Value* value, llvm::Value* low, llvm::Value* hight);

    /// Fits an integer value into a range of bits. All three arguments must
    /// be of same bitwidth.
    ///
    /// value: The original integer value.
    ///
    /// low: The number of the range's low bit.
    ///
    /// high: The number of the range's high bit.
    ///
    /// Returns: The result  where now *value* is shifted so that it spans from *low* to *high*.
    llvm::Value* llvmInsertBits(llvm::Value* value, llvm::Value* low, llvm::Value* hight);

    /// Similar to llvmDtor but postpones the operation until the current
    /// instruction has been finished.
    ///
    /// val: The value which's reference count to increase.
    ///
    /// type: The HILTI type of *val*.
    ///
    /// is_ptr: True if *val* is in fact a pointer to an instance of the type
    /// described by *type*; false if it's the instance itself.
    ///
    /// TODO: Do we still need this?
    void llvmDtorAfterInstruction(llvm::Value* val, shared_ptr<Type> type, bool is_ptr,
                                  bool is_hoisted, const string& location_addl);

    /// XXX.
    void llvmDtor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr,
                  const string& location_addl);

    /// XXX. For heap values only. Runs their dtor without releasing memory.
    void llvmDestroy(llvm::Value* val, shared_ptr<Type> type, const string& location_addl);

    /// XXX.
    void llvmCctor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr,
                   const string& location_addl);

    /// XXXX
    void llvmGCAssign(llvm::Value* dst, llvm::Value* val, shared_ptr<Type> type, bool plusone,
                      bool dtor_first = true);

    /// XXXX
    void llvmGCClear(llvm::Value* addr, shared_ptr<Type> type, const string& tag);

    void llvmClearLocalAfterInstruction(shared_ptr<Expression> expr, const string& location_addl);

    /// XXXX
    void llvmClearLocalOnException(shared_ptr<Expression> expr);

    /// XXXX
    void llvmFlushLocalsClearedOnException();

    /// XXXX
    void llvmMemorySafepoint(const std::string& where);

    /// XXXX Creates a stackmap for the current code location.
    void llvmCreateStackmap();

    /// XXX Ref/unref locals for get ref counts correct.
    void llvmAdaptStackForSafepoint(bool pre);

    /// XXX Returns all values currently live with theior types.
    typedef std::tuple<llvm::Value*, shared_ptr<Type>, bool> live_value;
    typedef std::list<live_value> live_list;
    live_list liveValues();

    /// XXXX
    void llvmDebugPrint(const string& stream, const string& msg);

    /// Increases the indentation level for debugging output.
    void llvmDebugPushIndent();

    /// Decreases the indentation level for debugging output.
    void llvmDebugPopIndent();

    /// Prints a string to stderr at runtime.
    void llvmDebugPrintString(const string& str);

    /// Prints a pointer to stderr at runtime, prefixed with an additional string.
    void llvmDebugPrintPointer(const string& prefix, llvm::Value* ptr);

    /// Print a HILTI object to stderr at runtime, prefixed with an additional string.
    void llvmDebugPrintObject(const string& prefix, llvm::Value* ptr, shared_ptr<Type> type);

    /// A case for llvmSwitch().
    struct SwitchCase {
        // FIXME: We should be able to use just a nromal function pointer here
        // but that doesn't compile with clang at the time of writing.
        typedef std::function<llvm::Value*(CodeGen* cg)> callback_t;

        SwitchCase(const string& l, std::list<llvm::ConstantInt*> o, callback_t c)
        {
            label = l;
            op_integers = o;
            callback = c;
            _enums = false;
        }

        SwitchCase(const string& l, llvm::ConstantInt* o, callback_t c)
        {
            label = l;
            op_integers.push_back(o);
            callback = c;
            _enums = false;
        }

        SwitchCase(const string& l, std::list<llvm::Value*> o, callback_t c)
        {
            label = l;
            op_enums = o;
            callback = c;
            _enums = true;
        }

        SwitchCase(const string& l, llvm::Value* o, callback_t c)
        {
            label = l;
            op_enums.push_back(o);
            callback = c;
            _enums = true;
        }

        /// A label used as part of the LLVM block name for the switch.
        string label;

        /// The case's value. Used only for llvmSwitch.
        std::list<llvm::ConstantInt*> op_integers;

        /// The case's value. Used only for llvmSwitchEnumConst. This must be
        /// a constant. The value returned by llvmEnum() works.
        std::list<llvm::Value*> op_enums;

        /// A callback run to build the case's code. The callback receives two
        /// parameters: \a cg is the code generator to use, with the current
        /// builder suitably set. If llvmSwitch() is called with \a result
        /// true, the callback must return the corresponding value; otherwise
        /// it must return null.
        callback_t callback;

        // Internal flag indicating what union element is set.
        bool _enums;
    };

    /// A list of cases for llvmSwitch(). The first element is a list
    /// of pair (lable, values) triggering execution of the case (which must
    /// of integer type); the second is the callback to build the code for the
    /// case.  The labels are used for naming the LLVM
    /// case-blocks.
    typedef std::list<SwitchCase> case_list;

    /// Builds an LLVM switch statement for integer operands. For each case,
    /// the caller provides a function that will be called to build the case's
    /// body. The helper will build the necessary code around the switch,
    /// generate an exception if the switch operand comes with an unknown
    /// case, and push a builder on the stack for code following the switch
    /// statement.
    ///
    /// op: The operand to switch on, which must be of integer type.
    ///
    /// cases: List of cases to build.
    ///
    /// result: If true, the callbacks in \a cases are expected to return
    /// values, which must be all of the same LLVM type. These are then merged
    /// with an LLVM ``phi`` instruction to select the one from the branch
    /// being taken, and the ``phi`` result is returned by this method.
    ///
    /// l: Location information to associate with the switch in case of an
    /// error.
    ///
    /// Returns: A value if \a result is true, null otherwise.
    ///
    /// \todo The function should optimized for the case where \a op is
    /// constant and then generate just the code for the corresponding branch.
    llvm::Value* llvmSwitch(llvm::Value* op, const case_list& cases, bool result = false,
                            const Location& l = Location::None);

    /// Builds an LLVM switch statement for enum operands. Generally, this
    /// works similar to llvmSwitch(), see there for more information. The
    /// difference is that it handles enums directly (which internally aren't
    /// just simple integers).
    ///
    /// op: The operand to switch on, which must be of enum type.
    ///
    /// cases: List of cases to build.
    ///
    /// result: If true, the callbacks in \a cases are expected to return
    /// values, which must be all of the same LLVM type. These are then merged
    /// with an LLVM ``phi`` instruction to select the one from the branch
    /// being taken, and the ``phi`` result is returned by this method.
    ///
    /// l: Location information to associate with the switch in case of an
    /// error.
    ///
    /// Returns: A value if \a result is true, null otherwise.
    ///
    /// \todo The function should optimized for the case where \a op is
    /// constant and then generate just the code for the corresponding branch.
    llvm::Value* llvmSwitchEnumConst(llvm::Value* op, const case_list& cases, bool result = false,
                                     const Location& l = Location::None);

    // Create the externally visible C functions wrapping a HILTI function.
    //
    // Returns: the tuple for main entry function and resume functions.
    std::pair<llvm::Value*, llvm::Value*> llvmBuildCWrapper(shared_ptr<Function> func);

    // Builds code that needs to run just before the current function returns.
    void llvmBuildFunctionCleanup();

    // Builds code that needs to run after the current instruction. This must
    // be called after the instruction no longer needs any of its operands but
    // before control flow potentially diverts. The method is called
    // automatically by the code generator after each instruction's code is
    // completely generated, which is sufficient for most instructions (those
    // that don't divert control flow). For those it's not, they need to call
    // this method at the appropiate time themselves, but they must ensure
    // that the code is *always* executed exactly *once*. The method usually
    // shouldn't be called more than once for the same instruction; if it is,
    // the subsequent calls won't have any effect as each call flushes the
    // internal state of cleanup code to run (unless the default argument is changed).
    //
    // flush: If false, the state isn't flushed.
    void llvmBuildInstructionCleanup(bool flush = true, bool dont_do_locals = false);

    // XXX
    void llvmDiscardInstructionCleanup();

    /// Tell the next call of llvmCall()/llvmRunHook() to run
    /// llvmBuildInstructionCleanup() on return.
    void setInstructionCleanupAfterCall();

    /// Allocates and intializes a new struct instance.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// ref: True for returning at +1, false for +0. In the latter case, the
    /// result will have been added to the context's nullbuffer so that at the
    /// next safepoint it will be deleted if not further refs take place.
    ///
    /// Returns: The newly allocated instance.
    llvm::Value* llvmStructNew(shared_ptr<Type> stype, bool ref);

    // A callback to filter the value returned by llvmStructGet().  The
    // callback will be called with the value extracted from the struct
    // instance and the returns value of the callback will then be used
    // subsequently instead of the field's value.
    //
    // FIXME: We should be able to use just a nromal function pointer here
    // but that doesn't compile with clang at the time of writing.
    typedef std::function<llvm::Value*(CodeGen* cg, llvm::Value* v)> struct_get_filter_callback_t;

    // A callback to provide a default for unset fields with llvmStructGet().
    //
    // FIXME: We should be able to use just a nromal function pointer here
    // but that doesn't compile with clang at the time of writing.
    typedef std::function<llvm::Value*(CodeGen* cg)> struct_get_default_callback_t;

    /// Extracts a field from a struct instance.
    ///
    /// The generated code will raise an \a UndefinedValue exception if the
    /// field is not set and no default has been provided.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance.
    ///
    /// field: The name of the field to extract.
    ///
    /// default: An optional default value to return if the field is not set,
    /// or null for no default.
    ///
    /// cb: A callback function to filter the extracted value.
    ///
    /// l: A location to associate with the exception being generated for unset fields.
    ///
    /// Returns: The field's value, or the default if provided and the field
    /// is not set, at +0.
    llvm::Value* llvmStructGet(shared_ptr<Type> stype, llvm::Value* sval, const string& field,
                               struct_get_default_callback_t default_,
                               struct_get_filter_callback_t filter,
                               const Location& l = Location::None);

    /// Extracts a field from a struct instance.
    ///
    /// The generated code will raise an \a UndefinedValue exception if the
    /// field is not set and no default has been provided.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance.
    ///
    /// field: The index of the field to extract.
    ///
    /// default: An optional default value to return if the field is not set,
    /// or null for no default.
    ///
    /// cb: A callback function to filter the extracted value.
    ///
    /// l: A location to associate with the exception being generated for unset fields.
    ///
    /// Returns: The field's value, or the default if provided and the field
    /// is not set, at +0.
    llvm::Value* llvmStructGet(shared_ptr<Type> stype, llvm::Value* sval, int field,
                               struct_get_default_callback_t default_,
                               struct_get_filter_callback_t filter,
                               const Location& l = Location::None);

    /// Sets a struct field.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance at +0.
    ///
    /// field: The index of the field to set.
    ///
    /// val: The value to set the field to. The value must not have its cctor called yet.
    void llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, int field, llvm::Value* val);

    /// Sets a struct field. This version gets an expression and coerce the
    /// type as necessary.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance at +0.
    ///
    /// field: The index of the field to set.
    ///
    /// val: The value to set the field to.
    void llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, int field,
                       shared_ptr<Expression> val);

    /// Sets a struct field.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance at +0.
    ///
    /// field: The name of the field to set.
    ///
    /// val: The value to set the field to. The value must not have its cctor called yet.
    void llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field,
                       llvm::Value* val);

    /// Sets a struct field. This version gets an expression and coerce the
    /// type as necessary.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance at +0.
    ///
    /// field: The name of the field to set.
    ///
    /// val: The value to set the field to.
    void llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field,
                       shared_ptr<Expression> val);

    /// Unsets a struct field.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance.
    ///
    /// field: The name of the field to set.
    void llvmStructUnset(shared_ptr<Type> stype, llvm::Value* sval, const string& field);

    /// Unsets a struct field.
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance.
    ///
    /// field: The index of the field to unset.
    void llvmStructUnset(shared_ptr<Type> stype, llvm::Value* sval, int field);

    /// Return a boolean value indicating whether a struct field is set. If
    /// the field has a default, it is always considered set (unless
    /// llvmStructUnset() is called explicitly).
    ///
    /// stype: The type, which must be a \a type::Struct.
    ///
    /// sval: The struct instance.
    ///
    /// field: The name of the field to set.
    ///
    /// Returns: A boolean LLVM value indicating whether the field is set.
    llvm::Value* llvmStructIsSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field);

    /// Extracts an element out of a tuple.
    ///
    /// ttype: The tuple type. This can be left null if \a cctor is false.
    ///
    /// tval: The tuple value.
    ///
    /// idx: The index of the element to extract.
    ///
    /// cctor: True if the returned value should have its cctor called already.
    ///
    /// Returns: The field's value, or the default if provided and the field
    /// is not set.
    llvm::Value* llvmTupleElement(shared_ptr<Type> ttype, llvm::Value* tval, int idx, bool cctor);

    typedef std::list<std::pair<shared_ptr<Type>, llvm::Value*>> element_list;

    /// Creates a tuple from a givem list of elements. All elements are
    /// coerced to the match the given tuple type.
    ///
    /// ttype: The target tuple type.
    ///
    /// elems: The tuple elements.
    ///
    /// cctor: If true, the returned value will have the cctor applied for all
    /// tuple elements.
    ///
    /// Returns: The new tuple.
    llvm::Value* llvmTuple(shared_ptr<Type> ttype, const element_list& elems, bool cctor);

    typedef std::list<shared_ptr<Expression>> expression_list;

    /// Creates a tuple from a givem list of elements. All elements are
    /// coerced to the match the given tuple type.
    ///
    /// ttype: The target tuple type.
    ///
    /// elems: The tuple elements.
    ///
    /// cctor: If true, the returned value will have the cctor applied for all
    /// tuple elements.
    ///
    /// Returns: The new tuple.
    llvm::Value* llvmTuple(shared_ptr<Type> ttype, const expression_list& elems, bool cctor);

    /// Creates a tuple from a givem list of elements. Note that there's no
    /// coercion, the LLVM types of the given tuple determine the type of the
    /// tuple directly. There's is also no cctor called for tuple elements
    /// (because we don't have their type infomration). If possible, you
    /// should use one of the other llvmTuple() variants.
    ///
    /// elems: The tuple elements.
    ///
    /// Returns: The new tuple at +0.
    llvm::Value* llvmTuple(const value_list& elems);

    /// Returns a a generic \a bytes end position.
    llvm::Value* llvmIterBytesEnd();

    /// Allocates a new instance of a memory managed object. The generated
    /// code is guaranteed to return a valid object; if we're running out of
    /// the memory the runtime will abort execution.
    ///
    /// type: The type of the object being allocaged. This must be of trait
    /// type::trait::GarbageCollected.
    ///
    /// llvm_type: The LLVM type to allocate an instance of. The size of this
    /// type determines how many bytes we allocate. This must be a struct type
    /// and it must begin with an instance of \c hlt.gchdr. Note that this
    /// needs to be given separately, because the type information for \c type
    /// will reference to the pointer value, not the object itself.
    ///
    /// ref: True for returning at +1, false for +0. In the latter case, the
    /// result will have been added to the context's nullbuffer so that at the
    /// next safepoint it will be deleted if not further refs take place.
    llvm::Value* llvmObjectNew(shared_ptr<Type> type, llvm::StructType* llvm_type, bool ref);

    /// Allocates an instance of the given LLVM type. This generated code is
    /// guaranteed to return a valid object; if we're running out of the
    /// memory the runtime will abort execution.
    ///
    /// ty: The type to allocate an instance of.
    ///
    /// type: A string describing the type being allocated. This is for debugging only.
    ///
    /// l: A location associated with the allocation. This is for debugging only.
    ///
    /// Returns: Pointer to the newly allocate object.
    llvm::Value* llvmMalloc(llvm::Type* ty, const string& type, const Location& l = Location::None);

    /// Allocates a given number of bytes. This generated code is guaranteed
    /// to return a valid object; if we're running out of the memory the
    /// runtime will abort execution.
    ///
    /// val: an i64 giving the number of bytes to allocate.
    ///
    /// type: A string describing the type being allocated. This is for debugging only.
    ///
    /// l: A location associated with the allocation. This is for debugging only.
    ///
    /// Returns: Pointer to the newly allocate object.
    llvm::Value* llvmMalloc(llvm::Value* size, const string& type,
                            const Location& l = Location::None);

    /// Release memory allocated with llvmMalloc().
    ///
    /// val: The allocated memory.
    ///
    /// type: A string describing the type being deallocated. This is for
    /// debugging only.
    ///
    /// l: A location associated with the deallocation. This is for debugging
    /// only.
    ///
    /// Returns: Pointer to the newly allocate object.
    void llvmFree(llvm::Value* val, const string& type, const Location& l = Location::None);

    /// Returns a LLVM value representing a classifier's field, intialized
    /// from a value.
    ///
    /// field_type: The type of the field; must of trait trait::Classifiable.
    ///
    /// src_type: The type of the value to initialize it with. The type must
    /// one of the ones that \a field_type's matchableTypes() returns.
    ///
    /// src_val: The value to initialize it with.
    ///
    /// Returns: A pointer to a ``hlt.classifier.field``. Passes ownership for
    /// the newly allocated object and its elements to the calling code.
    llvm::Value* llvmClassifierField(shared_ptr<Type> field_type, shared_ptr<Type> src_type,
                                     llvm::Value* src_val, const Location& l = Location::None);

    /// Returns a LLVM value representing a classifier's field, intialized
    /// from a byte sequence.
    ///
    /// data: A \a i8 pointer to bytes for the classifier to match on. The
    ///  *data* can become invalid after return from this method; if
    ///  necessary, bytes will be copied.
    ///
    /// len: Number of bytes that \a data is pointing to.
    ///
    /// bits: Optional number of *bits* valid in the byte sequence pointed to
    /// by *data*; the number must be equal or less than *len* times eight. If
    /// not given, all bytes will be matched on.
    ///
    /// Returns: A pointer to a ``hlt.classifier.field``. Passes ownership for
    /// the newly allocated object to the calling code.
    llvm::Value* llvmClassifierField(llvm::Value* data, llvm::Value* len,
                                     llvm::Value* bits = nullptr,
                                     const Location& l = Location::None);

    /// Generates code equivalnt to a \a memcpy call.
    ///
    /// src: The source address.
    ///
    /// dst: The destination address.
    ///
    /// n: The number of bytes to copy from \a src to \a dst.
    void llvmMemcpy(llvm::Value* dst, llvm::Value* src, llvm::Value* n);

    /// Generates code that compares to equally sized chunks of memory.
    ///
    /// p1: Pointer to 1st memory block.
    ///
    /// p2: Pointer to 2nd memory block.
    ///
    /// n: The number of bytes to cmp.
    ///
    /// Returns: An LLVM i1 that's true if the two blocks are byte-equivalent.
    llvm::Value* llvmMemEqual(llvm::Value* p1, llvm::Value* p2, llvm::Value* n);

    /// Converts an LLVM integer value from host to network bytes order. This
    /// methods suports 8/16/32/64 values.
    ///
    /// val: The LLVM integer.
    ///
    /// Returns: The converted integer.
    llvm::Value* llvmHtoN(llvm::Value* val);

    /// Converts an LLVM integer value from host to network bytes order. This
    /// methods suports 8/16/32/64 values.
    ///
    /// val: The LLVM integer.
    ///
    /// Returns: The converted integer.
    llvm::Value* llvmNtoH(llvm::Value* val);

    /// Evaluates a HILTI instruction. This version is for instructions
    /// without targets.
    void llvmInstruction(shared_ptr<Instruction> instr, shared_ptr<Expression> op1 = nullptr,
                         shared_ptr<Expression> op2 = nullptr, shared_ptr<Expression> op3 = nullptr,
                         const Location& l = Location::None);

    /// Evaluates a HILTI instruction. This version is for instructions
    /// with targets.
    void llvmInstruction(shared_ptr<Expression> target, shared_ptr<Instruction> instr,
                         shared_ptr<Expression> op1 = nullptr, shared_ptr<Expression> op2 = nullptr,
                         shared_ptr<Expression> op3 = nullptr, const Location& l = Location::None);

    /// Creates a local variable to be used as instructions operands. This
    /// will always create a new, unique local; the name may be appropiately
    /// adapted.
    ///
    /// name: The name of the local.
    ///
    /// type: The type of the local.
    ///
    /// Returns: An expression referencing the local.
    shared_ptr<hilti::Expression> makeLocal(const string& name, shared_ptr<Type> type,
                                            const AttributeSet& attrs = AttributeSet());

    /// Returns the LLVM function that initialized the module.
    llvm::Function* llvmModuleInitFunction();

    typedef std::function<llvm::Value*(CodeGen* cg, statement::Instruction*)> try_func;
    typedef std::function<void(CodeGen* cg, statement::Instruction*, llvm::Value* val)> finish_func;

    /// Creates umbrella code for an instruction that can block. If it does,
    /// the umbrella code yields back to the scheduler and tries the operation
    /// again next time it receives control back.
    ///
    /// i: The instruction.
    ///
    /// try_: A function that will be called to generate the LLVM code to try
    /// the operation one time. If the operation fails (i.e., the code should
    /// yield), it needs to raise a \a WouldBlock exception. Note that the
    /// generated code will run multiple times if control comes back. The
    /// function receives two parameters, the code generator and the
    /// instruction, and must return an LLVM value which's interpretation is
    /// left to the caller (it will be passed on to \a finish). Note that this
    /// code must make sure to not already handle the \a WouldBlock exception.
    /// In particular, when calling a C function, do not do any exception
    /// check; the umbrella code generated by the method will insert that but
    /// catch the \a WouldBlock first.
    ///
    /// finish: A function that will be called to generate LLVM code run once
    /// the \a try cide has successfully completed (i.e., not WouldBlock
    /// exception raised). The function receives three parameters: the code
    /// generator, the instruction, and the LLVM value return by the \a try.
    ///
    /// blockable_ty: The type of a blockable resource to wait for before
    /// giving control back; this must be a type of trait
    /// type::trait::Blockable. If given, \a blockable_val must be so as well.
    ///
    /// blockable_val: The value for the blockable resources, i.e., an
    /// instance of type \a blockable_ty.
    void llvmBlockingInstruction(statement::Instruction* i, try_func try_, finish_func finish,
                                 shared_ptr<Type> blockable_ty = nullptr,
                                 llvm::Value* blockable_val = nullptr);

    /// Starts a new profiler. The semantics of this method correspond
    /// directly to that of \c profiler.start, with defaults chosen in the
    /// same way. See the instruction documentation for more information.
    ///
    /// tag: A string value with the profiler's tag.
    ///
    /// style: A enum value with the type of profiler.
    ///
    /// param: The argument for the \a style, if needed. Must be an int64.
    ///
    /// tmgr: A timer manager to associate with the profiler.
    void llvmProfilerStart(llvm::Value* tag, llvm::Value* style = nullptr,
                           llvm::Value* param = nullptr, llvm::Value* tmgr = nullptr);

    /// Starts a new profiler. The semantics of this method correspond
    /// directly to that of \c profiler.start, with defaults chosen in the
    /// same way. See the instruction documentation for more information.
    ///
    /// tag: A string value with the profiler's tag.
    ///
    /// style: A fully qualified enum label with the type of profiler.
    ///
    /// param: The argument for the \a style, if needed.
    void llvmProfilerStart(const string& tag, const string& style = "", int64_t param = 0,
                           llvm::Value* tmgr = nullptr);

    /// Stops a profiler.
    ///
    /// tag: A string value with the profiler's tag.
    void llvmProfilerStop(llvm::Value* tag);

    /// Stops a profiler.
    ///
    /// tag: A string value with the profiler's tag.
    void llvmProfilerStop(const string& tag);

    /// Updates a profiler.
    ///
    /// tag: A string value with the profiler's tag.
    ///
    /// arg: The argument for the update. Must be an int64.
    void llvmProfilerUpdate(llvm::Value* tag, llvm::Value* arg);

    /// Updates a profiler.
    ///
    /// tag: A string value with the profiler's tag.
    ///
    /// arg: The argument for the update.
    void llvmProfilerUpdate(const string& tag, int64_t arg);

    /// XXX
    void prepareCall(shared_ptr<Expression> func, shared_ptr<Expression> args,
                     CodeGen::expr_list* call_params, bool before_call);

    // XXX
    string linkerModuleIdentifier() const;

    // XXX
    static string linkerModuleIdentifierStatic(llvm::Module* module);

    /// Returns the name of an LLVM module. This first looks for corresponding
    /// meta-data that the code generator inserts and returns that if found,
    /// and the standard LLVM module name otherwise. The meta data is helpful
    /// because LLVM's bitcode represenation doesn't preserve a module's name.
    static string llvmGetModuleIdentifier(llvm::Module* module);

private:
    // Creates/finishes the module intialization function that will receive all global
    // code, and pushes it onto the stack.
    void createInitFunction();
    void finishInitFunction();

    // Collects all globals and builds the data structures for accessing them,
    void initGlobals();

    // Creates a function that initializes the module's global variables.
    void createGlobalsInitFunction();

    // Creates the module-specific information for our custom linker pass.
    void createLinkerData();

    // Creates exported type information.
    void createRtti();

    // Creates a new function "name(void* ctx)" that the linker can later join
    // with other functions of the same signature. Inside the function, ctx
    // will be cast to an execution context (see method for more information).
    llvm::Function* llvmPushLinkerJoinableFunction(const string& name);

    // Fills the current funtions's exit block and links it with all the
    // exit_points via a PHI instruction.
    void llvmBuildExitBlock();

    /// Iterates over the current function's block and remove empty ones.
    void llvmNormalizeBlocks();

    // Implements both ref/unref.
    void llvmRefHelper(const char* func, llvm::Value* val, shared_ptr<Type> type);

    // Triggers local exception handling. If known_exception is true, we
    // already know that an exception has been raised; if false, we check for
    // it first.
    void llvmTriggerExceptionHandling(bool known_exception);

    // Helper that implements both llvmCall() and llvmRunHook().
    llvm::Value* llvmDoCall(llvm::Value* llvm_func, shared_ptr<Function> func,
                            shared_ptr<Hook> hook, shared_ptr<type::Function> ftype,
                            const expr_list& args, bool result_cctor, llvm::Value* hook_result,
                            bool excpt_check,
                            call_exception_callback_t excpt_callback = [](CodeGen* cg) {});

    // Take a type from libhilti.ll and adapts it for use in the current
    // module. See implementation of llvmLibFunction() for more information.
    llvm::Type* replaceLibType(llvm::Type* ntype);

    // In debug mode, returns an LLVM i8* value pointing to a string
    // describing the given location. In non-debug, returns 0.
    llvm::Value* llvmLocationString(const Location& l);

    // Returns a string with the location we're currently compiling in the
    // source file, if known (and if not, returns a place holder string).
    //
    // addl: An optional additional string to be included in the location
    // string.
    llvm::Value* llvmCurrentLocation(const string& addl = "");

    // Helper that implements the llvmCallableBind() methods.
    llvm::Value* llvmDoCallableBind(llvm::Value* llvm_func, shared_ptr<Function> func,
                                    shared_ptr<Hook> hook, shared_ptr<type::Function> ftype,
                                    const expr_list args, bool result_cctor,
                                    bool excpt_check = true, bool deep_copy_args = false,
                                    bool cctor_callable = false);

    // Helpers for llvmCallableBind() that builds the hlt.callable.func
    // object.
    llvm::Value* llvmCallableMakeFuncs(llvm::Function* llvm_func, shared_ptr<Function> func,
                                       shared_ptr<Hook> hook, shared_ptr<type::Function> ftype,
                                       bool result_cctor, bool excpt_check, llvm::StructType* sty,
                                       const string& name,
                                       const type::function::parameter_list& unbound_args);

    // Helper for calling hlt_string_from_data().
    llvm::Value* llvmStringFromData(const string& s);

    // An internal version of llvmInstruction that looks up the instruction by
    // name.
    void llvmInstruction(shared_ptr<Expression> target, const string& mnemo,
                         shared_ptr<Expression> op1, shared_ptr<Expression> op2,
                         shared_ptr<Expression> op3, const Location& l = Location::None);

    // Helper factoring out joint code for for llvmAddFunction() and llvmFunctionType().
    std::pair<llvm::Type*, std::vector<std::pair<string, llvm::Type*>>> llvmAdaptFunctionArgs(
        llvm::Type* rtype, parameter_list params, type::function::CallingConvention cc,
        bool skip_ctx);

    // Helper factoring out joint code for for llvmAddFunction() and llvmFunctionType().
    std::pair<llvm::Type*, std::vector<std::pair<string, llvm::Type*>>> llvmAdaptFunctionArgs(
        shared_ptr<type::Function> ftype);

    friend class util::IRInserter;
    const string& nextComment() const
    { // Used by the util::IRInserter.
        return _functions.back()->next_comment;
    }

    void clearNextComment()
    { // Used by the util::IRInserter.
        _functions.back()->next_comment.clear();
    }

    void setLeaveFunc(declaration::Function* f)
    {
        _functions.back()->leave_func = f;
    }

    friend class StatementBuilder;
    void pushExceptionHandler(shared_ptr<Expression> block, shared_ptr<type::Exception> type)
    { // Used by stmt-builder::Begin/EndHandler.
        _functions.back()->catches.push_front(std::make_pair(block, type));
    }

    void popExceptionHandler()
    { // Used by stmt-builder::Try/Catch.
        _functions.back()->catches.pop_front();
    }

    // Pushing a nullptr disables end of block handling.
    void pushEndOfBlockHandler(IRBuilder* b)
    {
        _functions.back()->handle_block_end.push_back(b);
    }

    void popEndOfBlockHandler()
    {
        _functions.back()->handle_block_end.pop_back();
    }

    std::pair<bool, IRBuilder*> topEndOfBlockHandler();

    void llvmRunDtorsAfterIns();

    path_list _libdirs;

    // int _in_check_exception = 0;
    int _in_build_exit = 0;

    unique_ptr<Loader> _loader;
    unique_ptr<Storer> _storer;
    unique_ptr<Unpacker> _unpacker;
    unique_ptr<Packer> _packer;
    unique_ptr<FieldBuilder> _field_builder;
    unique_ptr<StatementBuilder> _stmt_builder;
    unique_ptr<Coercer> _coercer;
    unique_ptr<TypeBuilder> _type_builder;
    unique_ptr<DebugInfoBuilder> _debug_info_builder;
    unique_ptr<passes::Collector> _collector;
    unique_ptr<ABI> _abi;
    std::unique_ptr<llvm::Module> _libhilti = nullptr;
    std::unique_ptr<llvm::Module> _module = nullptr;

    shared_ptr<hilti::Module> _hilti_module = nullptr;
    CompilerContext* _ctx = nullptr;
    llvm::DataLayout* _data_layout = nullptr;
    llvm::Function* _module_init_func = nullptr;
    llvm::Function* _globals_init_func = nullptr;
    llvm::Function* _globals_dtor_func = nullptr;
    llvm::Value* _globals_base_func = nullptr;
    llvm::Type* _globals_type = nullptr;

    typedef std::list<std::pair<shared_ptr<Expression>, shared_ptr<type::Exception>>> handler_list;
    typedef std::list<IRBuilder*> builder_list;
    typedef std::map<string, int> label_map;
    typedef std::map<string, IRBuilder*> builder_map;
    typedef std::map<string, std::tuple<llvm::Value*, shared_ptr<Type>, bool>> local_map;
    typedef std::multimap<shared_ptr<Statement>,
                          std::tuple<llvm::Value*, bool, shared_ptr<Type>, bool, bool, string>>
        dtor_map;
    typedef std::multimap<shared_ptr<Statement>, std::tuple<shared_ptr<Expression>, string>>
        dtor_expr_map;
    typedef std::list<std::pair<llvm::BasicBlock*, llvm::Value*>> exit_point_list;

    struct FunctionState {
        llvm::Function* function;
        llvm::BasicBlock* exit_block = nullptr;
        builder_list builders;
        builder_list handle_block_end;
        builder_map builders_by_name;
        label_map labels;
        local_map locals;
        local_map tmps;
        exit_point_list exits;
        dtor_map dtors_after_ins;
        dtor_expr_map dtors_after_ins_exprs;
        bool dtors_after_call; // If true, run dtors_after_ins for the current statement after next
                               // llvmCall().
        string next_comment;
        bool abort_on_excpt;
        bool is_init_func;
        llvm::Value* context;
        declaration::Function* leave_func = nullptr;
        std::list<shared_ptr<Expression>> locals_cleared_on_excpt;
        handler_list catches;
        type::function::CallingConvention cc;
        int stackmap_id = 0;
    };

    typedef std::list<std::unique_ptr<FunctionState>> function_list;
    function_list _functions;

    typedef std::map<string, llvm::Value*> value_cache_map;
    value_cache_map _value_cache;

    typedef std::map<string, llvm::Constant*> constant_cache_map;
    constant_cache_map _constant_cache;

    typedef std::map<string, llvm::Type*> type_cache_map;
    type_cache_map _type_cache;

    typedef std::map<string, int> name_map;
    name_map _unique_names;

    typedef std::map<Variable*, llvm::Value*> globals_map;
    globals_map _globals;

    typedef std::list<std::pair<string, llvm::Value*>> global_string_list;
    global_string_list _global_strings;
};

/// Base class for visitor that the code generator uses for its work.
template <typename Result = int, typename Arg1 = int, typename Arg2 = int>
class CGVisitor : public hilti::Visitor<Result, Arg1, Arg2> {
public:
    /// Constructor.
    ///
    /// cg: The code generator the visitor is used by.
    ///
    /// logger_name: An identifying name passed on the Logger framework.
    CGVisitor(CodeGen* cg, const string& logger_name)
    {
        _codegen = cg;
        this->forwardLoggingTo(cg);
        this->setLoggerName(logger_name);
    }

    /// Returns the code generator this visitor is attached to.
    CodeGen* cg() const
    {
        return _codegen;
    }

    /// Returns the current builder, retrieved from the code generator. This
    /// is just a shortcut for calling CodeGen::builder().
    ///
    /// Returns: The current LLVM builder.
    IRBuilder* builder() const
    {
        return _codegen->builder();
    }

private:
    CodeGen* _codegen;
};
}
}

#endif
