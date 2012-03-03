
#ifndef HILTI_CODEGEN_CODEGEN_H
#define HILTI_CODEGEN_CODEGEN_H

#include <list>

#include "common.h"
#include "util.h"
#include "../type.h"
#include "../visitor.h"

namespace hilti {

class Module;
class Type;
class Expression;
class Variable;
class LogComponent;

namespace statement { class Instruction; }
namespace passes { class Collector; }

namespace codegen {

namespace util { class IRInserter; }

class TypeInfo;

/// Namespace for the name of symbols generated/examined by code generator
/// and linker.
namespace symbols {
    // Names of meta data examined by AssemblyAnnotationWriter.
    static const char* MetaComment      = "hlt.comment";

    // Names of globals examined by custom linker pass.
    static const char* MetaModule       = "hlt.module";
    static const char* MetaModuleInit   = "hlt.module.init";
    static const char* MetaGlobalsInit  = "hlt.globals.init";
    static const char* MetaGlobalsDtor  = "hlt.globals.dtor";

    static const char* TypeGlobals      = "hlt.globals.type";
    static const char* FuncGlobalsBase  = "hlt.globals.base";

    // Symbols created by the linker for access by libhilti.
    static const char* FunctionGlobalsInit = "__hlt_globals_init";
    static const char* FunctionGlobalsDtor = "__hlt_globals_dtor";
    static const char* FunctionModulesInit = "__hlt_module_init";
    static const char* ConstantGlobalsSize = "__hlt_globals_size";

    // Names for argument added internally for our calling conventions.
    static const char* ArgExecutionContext = "__ctx";
    static const char* ArgException        = "__excpt";
}


/// Namespace for symbols that corresponds to, and must with, element defined
/// at the C layer in libhilti.
namespace hlt {
    /// Fields in %hlt.execution_context.
    enum ExecutionContext { Globals = 0 };

    /// Fields in %hlt.exception.
    enum Exception { Name = 0 };
}

class Coercer;
class Loader;
class Storer;
class Unpacker;
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
class CodeGen : public ast::Logger
{
public:
   /// Constructor.
   ///
   /// libdirs: Path where to find library modules that the code generator
   /// may need.
   ///
   /// cg: Debug level indicator the verbosity of the output the generator
   /// produces during code generation. The higher, the more output.
   CodeGen(const path_list& libdirs, int cg_debug_level);
   virtual ~CodeGen();

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
   /// Passes ownership to the caller.
   llvm::Module* generateLLVM(shared_ptr<hilti::Module> hltmod, bool verify, int debug, int profile);

   /// Returns the debug level for the generated code. This is the value
   /// passed into generateLLVM().
   int debugLevel() const { return _debug_level; }

   /// Returns the LLVM context to use with all LLVM calls.
   llvm::LLVMContext& llvmContext() { return llvm::getGlobalContext(); }

   /// Returns the LLVM builder to use for inserting code at the current
   /// location. For each LLVM function being built, a stack of builder is
   /// maintained and manipulated via pushBuilder() and popBuilder().
   IRBuilder* builder() const;

   /// Returns the LLVM function currently being generated.
   llvm::Function* function() const { return _functions.back()->function; }

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
   llvm::Function* pushFunction(llvm::Function* function, bool push_builder=true);

   /// Removes the current LLVM function from the stack of function being
   /// generated. Calls to this function must match with those to
   /// pushFunction().
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

   /// Adds a comment just before the next instruction that will be printed
   /// out by the AssemblyAnnotationWriter.
   void llvmInsertComment(const string& comment);

   /// Returns the LLVM module currently being built.
   llvm::Module* llvmModule() const { return _module; }

   /// Returns the LLVM module currently being compiled.
   shared_ptr<hilti::Module> hiltiModule() const { return _hilti_module; }

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
   llvm::Function* llvmFunction(shared_ptr<Function> func);

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

   /// Returns the LLVM value for a HILTI expression. The value will have its
   /// cctor function called already (if necessary) to create a new copy. If
   /// that isn't needed, llvmDtor() must be called on the result.
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
   llvm::Value* llvmValue(shared_ptr<Expression> expr, shared_ptr<hilti::Type> coerce_to = nullptr, bool cctor = false);

   /// Returns the LLVM value for the current execution context. Depending on
   /// what object currently being generated, the value may refer to
   /// different things.
   ///
   /// Returns: The LLVM value.
   llvm::Value* llvmExecutionContext();

   /// Returns the size of an LLVM constant. This is a helper function
   /// implementing a \c sizeof operator, which LLVM does not provide out of
   /// the box.
   llvm::Constant* llvmSizeOf(llvm::Constant* v);

   /// Returns the size of an LLVM type. This is a helper function
   /// implementing a \c sizeof operator, which LLVM does not provide out
   /// of the box.
   llvm::Constant* llvmSizeOf(llvm::Type* t);

   /// Stores an LLVM value at the location associated with a HILTI
   /// expression. The value must already have its cctor called, either
   /// implictly if its comming from llvmValue() or explicitly via
   /// llvmCCtor().
   ///
   /// This method branches out to the Storer to do its work.
   ///
   /// target: The expression to evaluate to get the location where to store
   /// the value.
   ///
   /// value: The value to store.
   void llvmStore(shared_ptr<hilti::Expression> target, llvm::Value* value);

   /// Stores an LLVM value at the location associated with a HILTI
   /// instruction's target operand. expression. The value must already have
   /// its cctor called, either implictly if its comming from llvmValue() or
   /// explicitly via llvmCCtor().
   ///
   /// This methods branches out to the Storer to do its work.
   ///
   /// instr: The instruction's which's target operand gives us the
   /// destination for the store operation.
   ///
   /// value: The value to store.
   void llvmStore(statement::Instruction* instr, llvm::Value* value);


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
   std::pair<llvm::Value*, llvm::Value*> llvmUnpack(
                   shared_ptr<Type> type, shared_ptr<Expression> begin,
                   shared_ptr<Expression> end, shared_ptr<Expression> fmt,
                   shared_ptr<Expression> arg = nullptr,
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
   std::pair<llvm::Value*, llvm::Value*> llvmUnpack(
                   shared_ptr<Type> type, llvm::Value* begin,
                   llvm::Value* end, llvm::Value* fmt,
                   llvm::Value* arg = nullptr,
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
   llvm::Value* llvmGlobalIndex(shared_ptr<Variable> var) { return llvmGlobalIndex(var.get()); }

   /// Coerces an LLVM value from one type into another. It assumes that the
   /// coercion is legal and supported. If not, results are undefined.
   ///
   /// The method uses the Coercer to do its work.
   ///
   /// value: The value to coerce.
   ///
   /// src: The source type.
   ///
   /// dst: The destination type.
   ///
   /// Returns: The coerce value.
   llvm::Value* llvmCoerceTo(llvm::Value* value, shared_ptr<hilti::Type> src, shared_ptr<hilti::Type> dst);

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
   shared_ptr<TypeInfo> typeInfo(shared_ptr<hilti::Type> type);

   /// Returns the LLVM type that corresponds to a HILTI type.
   ///
   /// This methods uses the TypeBuilder to do its work.
   ///
   /// type: The type to return the LLVM type for.
   ///
   /// Returns: The corresponding LLVM type.
   llvm::Type* llvmType(shared_ptr<hilti::Type> type);

   /// Returns the LLVM initialization value for a HILTI type.
   ///
   /// This methods uses the TypeBuilder to do its work.
   ///
   /// type: The type to return the initalization value for.
   ///
   /// Returns: The LLVM initialization value.
   llvm::Constant* llvmInitVal(shared_ptr<hilti::Type> type);

   /// Returns the LLVM RTTI object for a HILTI type.
   ///
   /// This methods uses the TypeBuilder to do its work.
   ///
   /// type: The type to return the RTTI for.
   ///
   /// Returns: The corresponding LLVM value.
   llvm::Constant* llvmRtti(shared_ptr<hilti::Type> type);

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
   llvm::Type* llvmTypeStruct(const string& name, const type_list& fields, bool packed=false); // name="" create anon struct.

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
   llvm::Constant* llvmConstStruct(const constant_list& elems, bool packed=false);

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
   /// elems: The list of elements to initialize the array with. Can be empty to create an empty array.
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

   /// Returns a constant representing a GetElementPtr index. This is just a
   /// shortcut to creating the constant "manually.
   ///
   /// idx: The index to initialize the constant with.
   llvm::Constant* llvmGEPIdx(int64_t idx);

   /// Returns the LLVM value corresponding to a HILTI string of given value.
   ///
   /// s: The string value.
   llvm::Value* llvmString(const string& s);

   /// Returns a pointer to the LLVM value corresponding to a HILTI string of
   /// given value.
   ///
   /// s: The string value.
   llvm::Value* llvmStringPtr(const string& s);

   /// Returns an LLVM struct value with fields initialized. There's also a
   /// version of this method that works with constants rather than arbitrary
   /// values, llvmConstStruct().
   ///
   /// elems: The field values.
   ///
   /// packed: True if the constant is to represent a packed struct type.
   llvm::Value* llvmValueStruct(const std::vector<llvm::Value*>& elems, bool packed=false);

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
   llvm::GlobalVariable* llvmAddConst(const string& name, llvm::Constant* val, bool use_name_as_is = false);

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
   llvm::GlobalVariable* llvmAddGlobal(const string& name, llvm::Type* type, llvm::Constant* init = 0, bool use_name_as_is = false);

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
   llvm::GlobalVariable* llvmAddGlobal(const string& name, llvm::Constant* init, bool use_name_as_is = false);

   /// Adds a new local variable to the current LLVM function. Locals created
   /// with this method should correspond to variables defined by the user at
   /// the HILTI level. To create internal temporaries for use by the code
   /// generator, use llvmAddTmp() instead. Locals will be unrefed
   /// automatically at function exit.
   ///
   /// name: A name to use for the local. Note that it's not necessarily used
   /// literally and may be adpated further.
   ///
   /// type: The type of the local.
   ///
   /// init: An optional intialization value.
   ///
   /// Returns: The new local variable.
   llvm::Value* llvmAddLocal(const string& name, shared_ptr<Type> type, llvm::Value* init = 0);

   /// Adds a new local temporary variable to the current LLVM function.
   /// Temporaries don't correspond to a user-defined variable but are
   /// created internally by the code generator to store intermediaries.
   /// Locals will *not* be unrefed automatically at function exit. If you
   /// need that, use llvmUnrefAtExit or llvmUnrefAfterInstruction.
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
   llvm::Value* llvmAddTmp(const string& name, llvm::Type* type, llvm::Value* init = 0, bool reuse = false);

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
   llvm::Value*    llvmAddTmp(const string& name, llvm::Value* init = 0, bool reuse = false);

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
   /// Returns: The new function.
   llvm::Function* llvmAddFunction(shared_ptr<Function> func, bool internal, type::function::CallingConvention cc = type::function::DEFAULT);

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
   /// Returns: The new function.
   llvm::Function* llvmAddFunction(const string& name, llvm::Type* rtype, parameter_list params, bool internal, type::function::CallingConvention cc);

   /// XXX.
   llvm::Function* llvmAddFunction(const string& name, llvm::Type* rtype, llvm_parameter_list params, bool internal);

   typedef std::vector<shared_ptr<Expression>> expr_list;

   /// Generates an LLVM call to a HILTI function. This method handles all
   /// calling conventions.  The return value will have its cctor function
   /// called already (if necessary) to create a new copy. If that isn't
   /// needed, llvmDtor() must be called on the result.
   ///
   /// func: The HILTI function to call.
   ///
   /// ftype: The type of the callee.
   ///
   /// args: The parameters to evaluate and pass to the callee.
   ///
   /// excpt_check: If false, the call will not be followed for a check
   /// whether the callee has raised an exception. Use that if you're sure
   /// that it won't, or if for some other reason you know it's not an issue.
   ///
   /// Returns: The call instruction created.
   llvm::CallInst* llvmCall(shared_ptr<Function> func, const expr_list& args, bool excpt_check=true);

   /// Generates an LLVM call to a HILTI function. This method handles all
   /// calling conventions. The return value will have its cctor function
   /// called already (if necessary) to create a new copy. If that isn't
   /// needed, llvmDtor() must be called on the result.
   ///
   /// llvm_func: The LLVM function corresponding to the HILTI function we
   /// want to call (usually retrieved via llvmFunction()).
   ///
   /// ftype: The type of the callee.
   ///
   /// args: The parameters to evaluate and pass to the callee.
   ///
   /// excpt_check: If false, the call will not be followed for a check
   /// whether the callee has raised an exception. Use that if you're sure
   /// that it won't, or if for some other reason you know it's not an issue.
   ///
   /// Returns: The call instruction created.
   llvm::CallInst* llvmCall(llvm::Value* llvm_func, shared_ptr<type::Function> ftype, const expr_list& args, bool excpt_check=true);

   /// Generates an LLVM call to a HILTI function. This method handles all
   /// calling conventions. The return value will have its cctor function
   /// called already (if necessary) to create a new copy. If that isn't
   /// needed, llvmDtor() must be called on the result.
   ///
   /// name: The name of the HILTI function to call, which must be resolvable
   /// in the current module's scope.
   ///
   /// ftype: The type of the callee.
   ///
   /// args: The parameters to evaluate and pass to the callee.
   ///
   /// excpt_check: If false, the call will not be followed for a check
   /// whether the callee has raised an exception. Use that if you're sure
   /// that it won't, or if for some other reason you know it's not an issue.
   ///
   /// Returns: The call instruction created.
   llvm::CallInst* llvmCall(const string& name, const expr_list& args, bool excpt_check=true);

   /// Generates the code for a HILTI \c return or \c return.void statement.
   /// This methods must be used instead of a plain LLVM \c return
   /// instruction; it makes sure to run any necessary end-of-function
   /// cleanup code.
   ///
   /// result: The result to return from the current function, or null if the
   /// return type is void.
   void llvmReturn(llvm::Value* result = 0);

   typedef std::vector<llvm::Value*> value_list;

   /// Generates a straight LLVM call to a C function, without adapting
   /// parameters to HILTI'c calling conventions. Note that return value will
   /// \a not have its cctor called.
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
   /// Returns: The call instruction created.
   llvm::CallInst* llvmCallC(llvm::Value* llvm_func, const value_list& args, bool add_hiltic_args, bool excpt_check=true);

   /// Generates a straight LLVM call to a C function defined in \c
   /// libhilti.ll, without adapting parameters to HILTI'c calling
   /// conventions. Note that return value will \a not have its cctor called.
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
   /// Returns: The call instruction created.
   llvm::CallInst* llvmCallC(const string& llvm_func, const value_list& args, bool add_hiltic_args, bool excpt_check=true);

   /// Calls an LLVM intrinsic.
   ///
   /// intr: The intrinsic's ``INTR_*`` enum constant. See \c
   /// include/llvm/Intrinsics.gen for list
   ///
   /// ty: The types determining the version to pick for overloaded intrinsics; empty if not needed.
   ///
   /// args: The arguments to call the intrinsic with.
   ///
   /// Returns: Whatever the intrinsic returns.
   llvm::CallInst* llvmCallIntrinsic(llvm::Intrinsic::ID intr, std::vector<llvm::Type*> tys, const value_list& args);

   ///i = llvm.core.Function.intrinsic(self._llvm.module, intr, types)
   ///return self.llvmSafeCall(i, args)


   /// Inserts code that checks with a \c C-HILTI function has raised an
   /// exception. If so, the code will reraise that as a HILTI exception.
   ///
   /// excpt: The value where the \c C-HILTI function would have stored its
   /// exception (i.e., usually the \c excpt parameter).
   void llvmCheckCException(llvm::Value* excpt);

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
   ///
   /// \todo As we have not implemented exception yet, this currently just
   /// aborts execution.
   void llvmRaiseException(const string& exception, shared_ptr<Node> node, llvm::Value* arg = nullptr);

   /// Generates code to raise an exception. When executed, the code will
   /// *not* return control back to the current block.
   ///
   /// exception: The name of the exception's type. The name must define an
   /// exception type in \c libhilti.ll.
   ///
   /// loc: Location information to associate with the exception.
   ///
   /// arg: The exception's argument if the type takes one, or null if not.
   ///
   /// \todo As we have not implemented exception yet, this currently just
   /// aborts execution.
   void llvmRaiseException(const string& exception, const Location& l,  llvm::Value* arg = nullptr);

   /// Generates code to raise an exception. When executed, the code will
   /// *not* return control back to the current block.
   ///
   /// excpt: The LLVM exception object to raise. The must have been cctored
   /// already.
   void llvmRaiseException(llvm::Value* excpt);

   /// Rethrows an already thrown exception by unwinding the stack until a
   /// handler is found.
   void llvmRethrowException();

   /// Clears the current exception.
   void llvmClearException();

   /// Extracts the current exception from the execution context.
   llvm::Value* llvmCurrentException();

   /// Checks whether a connection has been raised and if so, triggers
   /// handling. Control flow may not return in that case if stack unwinding
   /// is needed.
   void llvmCheckException();

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
   llvm::CallInst* llvmCreateCall(llvm::Value* callee, llvm::ArrayRef<llvm::Value *> args, const llvm::Twine &name="");

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
   llvm::CallInst* llvmCreateCall(llvm::Value* callee, const llvm::Twine &name="");

   /// Wrapper method to create an LLVM \c sotre instructions that first
   /// checks operands for compatibility. If not matching, it dumps out
   /// debugging outout and abort execution.
   llvm::StoreInst* llvmCreateStore(llvm::Value *val, llvm::Value *ptr, bool isVolatile=false);

   /// Wrapper method to create an LLVM \c branch instruction that takes
   /// builders instead of blocks.
   llvm::BranchInst* llvmCreateBr(IRBuilder* builder);

   /// Wrapper method to create an LLVM \c conditional branch instruction
   /// that takes builders instead of blocks.
   llvm::BranchInst* llvmCreateCondBr(llvm::Value* cond, IRBuilder* true_, IRBuilder* false_);

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
   llvm::Value*    llvmGEP(llvm::Value* addr, llvm::Value* i1, llvm::Value* i2 = 0, llvm::Value* i3 = 0, llvm::Value* i4 = 0);

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
   llvm::Constant* llvmGEP(llvm::Constant* addr, llvm::Value* i1, llvm::Value* i2 = 0, llvm::Value* i3 = 0, llvm::Value* i4 = 0);

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
   llvm::Value*    llvmInsertValue(llvm::Value* aggr, llvm::Value* val, unsigned int idx);

   /// Wrapper method to create an LLVM \c extract_value instruction a bit
   /// more easily. There's also a const version, llvmConstExtractValue().
   ///
   /// aggr: The aggregate value to extract from.
   ///
   /// idx: The index to extract.
   ///
   /// Returns: The extracted value.
   llvm::Value*    llvmExtractValue(llvm::Value* aggr, unsigned int idx);

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
   llvm::Constant* llvmConstInsertValue(llvm::Constant* aggr, llvm::Constant* val, unsigned int idx);

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

#if 0
   /// Increases the reference count of a garbage-collected object. This
   /// method is safe to call also instances of types that are not reference
   /// counted, and with null pointers; in both cases, it will just turn into
   /// a no-op (either statically or at runtime). If it gets called with a
   /// reference to a garbage collected object, that will be dereferenced
   /// first.
   ///
   /// val: The value which's reference count to increase.
   ///
   /// type: The HILTI type of *val*.
   void llvmRef(llvm::Value* val, shared_ptr<Type> type);

   /// Decreases the reference count of a garbage-collected object. If it
   /// goes to zero, the object will be freed and become invalid. This method
   /// is safe to call also instances of types that are not reference
   /// counted, and with null pointers; in both cases, it will just turn into
   /// a no-op (either statically or at runtime). If it gets called with a
   /// reference to a garbage collected object, that will be dereferenced
   /// first.
   ///
   /// val: The value which's reference count to increase.
   ///
   /// type: The HILTI type of *val*.
   void llvmUnref(llvm::Value* val, shared_ptr<Type> type);

   /// Similar to llvmUnref but postpones the counter decrease until the end
   /// of the current function.
   ///
   /// val: The value which's reference count to increase.
   ///
   /// type: The HILTI type of *val*.
   void llvmUnrefAtReturn(llvm::Value* val, shared_ptr<Type> type);

#endif

   /// Similar to llvmDtor but postpones the operation until the current
   /// instruction has been finished.
   ///
   /// val: The value which's reference count to increase.
   ///
   /// type: The HILTI type of *val*.
   ///
   /// is_ptr: True if *val* is in fact a pointer to an instance of the type
   /// described by *type*; false if it's the instance itself.
   void llvmDtorAfterInstruction(llvm::Value* val, shared_ptr<Type> type, bool is_ptr);

   /// XXX.
   void llvmDtor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr);

   /// XXX.
   void llvmCctor(llvm::Value* val, shared_ptr<Type> type, bool is_ptr);

   /// XXXX
   void llvmGCAssign(llvm::Value* dst, llvm::Value* val, shared_ptr<Type> type, bool plusone);

   /// XXXX
   void llvmGCClear(llvm::Value* val, shared_ptr<Type> type);

   /// Finished generation of a statement. This is called from the statement builder.
   void finishStatement();

   /// XXXX
   void llvmDebugPrint(const string& stream, const string& msg);


   /// A case for llvmSwitch().
   struct SwitchCase {
       // FIXME: We should be able to use just a nromal function pointer here
       // but that doesn't compile with clang at the time of writing.
       typedef std::function<llvm::Value* (CodeGen* cg)> callback_t;

       SwitchCase(const string& l, std::list<llvm::ConstantInt*> o, callback_t c) {
           label = l; op_integers = o; callback = c; _enums = false;
       }

       SwitchCase(const string& l, llvm::ConstantInt* o, callback_t c) {
           label = l; op_integers.push_back(o); callback = c; _enums = false;
       }

       SwitchCase(const string& l, std::list<llvm::Value*> o, callback_t c) {
           label = l; op_enums = o; callback = c; _enums = true;
       }

       SwitchCase(const string& l, llvm::Value* o, callback_t c) {
           label = l; op_enums.push_back(o); callback = c; _enums = true;
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
   llvm::Value* llvmSwitch(llvm::Value* op, const case_list& cases, bool result = false, const Location& l=Location::None);

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
   llvm::Value* llvmSwitchEnumConst(llvm::Value* op, const case_list& cases, bool result = false, const Location& l=Location::None);

   // Fills the current funtions's exit block and links it with all the
   // exit_points via a PHI instruction.
   void llvmBuildExitBlock();

   // Create the externally visible C function wrapping a HILTI function *internal*.
   void llvmBuildCWrapper(shared_ptr<Function> func, llvm::Function* internal);

   // Builds code that needs to run just before the current function returns.
   void llvmBuildFunctionCleanup();

   /// Allocates and intializes a new struct instance.
   ///
   /// stype: The type, which must be a \a type::Struct.
   ///
   /// Returns: The newly allocated instance.
   llvm::Value* llvmStructNew(shared_ptr<Type> stype);

   // A callback to filter the value returned by llvmStructGet().  The
   // callback will be called with the value extracted from the struct
   // instance and the returns value of the callback will then be used
   // subsequently instead of the field's value.
   typedef llvm::Value* (*StructGetCallBack)(CodeGen* cg, llvm::Value* v);

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
   /// is not set. The returned value will have its cctor called already.
   llvm::Value* llvmStructGet(shared_ptr<Type> stype, llvm::Value* sval, const string& field, llvm::Value* default_, StructGetCallBack* cb, const Location& l=Location::None);

   /// Sets a struct field.
   ///
   /// stype: The type, which must be a \a type::Struct.
   ///
   /// sval: The struct instance.
   ///
   /// field: The name of the field to set.
   ///
   /// val: The value to set the field to. The value must have its cctor called already.
   void llvmStructSet(shared_ptr<Type> stype, llvm::Value* sval, const string& field, llvm::Value* val);

   /// Unsets a struct field.
   ///
   /// stype: The type, which must be a \a type::Struct.
   ///
   /// sval: The struct instance.
   ///
   /// field: The name of the field to set.
   void llvmStructUnset(shared_ptr<Type> stype, llvm::Value* sval, const string& field);

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
   llvm::Value* llvmObjectNew(shared_ptr<Type> type, llvm::StructType* llvm_type);

#if 0
   /// Allocates an instance of the given LLVM type. This generated code is
   /// guaranteed to return a valid object; if we're running out of the
   /// memory the runtime will abort execution.
   ///
   /// ty: The type to allocate an instance of.
   ///
   /// Returns: Pointer to the newly allocate object.
   llvm::Value* llvmMalloc(llvm::Type* ty);
#endif

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

   // Implements both ref/unref.
   void llvmRefHelper(const char* func, llvm::Value* val, shared_ptr<Type> type);

   // Triggers local exception handling. If known_exception is true, we
   // already know that an exception has been raised; if false, we check for
   // it first.
   void llvmTriggerExceptionHandling(bool known_exception);

   // Take a type from libhilti.ll and adapts it for use in the current
   // module. See implementation of llvmLibFunction() for more information.
   llvm::Type* replaceLibType(llvm::Type* ntype);

   // In debug mode, returns an LLVM i8* value pointing to a string
   // describing the given location. In non-debug, returns 0.
   llvm::Value* llvmLocationString(const Location& l);

   friend class util::IRInserter;
   const string& nextComment() const { // Used by the util::IRInserter.
       return _functions.back()->next_comment;
   }

   void clearNextComment() { // Used by the util::IRInserter.
       _functions.back()->next_comment.clear();
   }

   friend class StatementBuilder;
   void pushExceptionHandler(IRBuilder* handler) { // Used by stmt-builder::Try/Catch.
       _functions.back()->catches.push_back(handler);
   }

   void popExceptionHandler() { // Used by stmt-builder::Try/Catch.
       _functions.back()->catches.pop_back();
   }

   IRBuilder* topExceptionHandler() { // Used by stmt-builder::Try/Catch.
       return _functions.back()->catches.back();
   }

   path_list _libdirs;
   bool _verify;
   int _debug_level;
   int _profile;

   int _in_check_exception = 0;

   unique_ptr<Loader> _loader;
   unique_ptr<Storer> _storer;
   unique_ptr<Unpacker> _unpacker;
   unique_ptr<StatementBuilder> _stmt_builder;
   unique_ptr<Coercer> _coercer;
   unique_ptr<TypeBuilder>  _type_builder;
   unique_ptr<DebugInfoBuilder> _debug_info_builder;
   unique_ptr<passes::Collector> _collector;

   shared_ptr<hilti::Module> _hilti_module = nullptr;
   llvm::Module* _libhilti = nullptr;
   llvm::Module* _module = nullptr;
   llvm::Function* _module_init_func = nullptr;
   llvm::Function* _globals_init_func = nullptr;
   llvm::Function* _globals_dtor_func = nullptr;
   llvm::Value* _globals_base_func = nullptr;
   llvm::Type* _globals_type = nullptr;

   typedef std::list<IRBuilder*> builder_list;
   typedef std::map<string, int> label_map;
   typedef std::map<string, IRBuilder*> builder_map;
   typedef std::map<string, std::pair<llvm::Value*, shared_ptr<Type>>> local_map;
   typedef std::list<std::pair<std::pair<llvm::Value*, bool>, shared_ptr<Type>>> dtor_list;
   typedef std::list<std::pair<llvm::BasicBlock*, llvm::Value*>> exit_point_list;

   struct FunctionState {
       llvm::Function* function;
       llvm::BasicBlock* exit_block = nullptr;
       builder_list builders;
       builder_map builders_by_name;
       label_map labels;
       local_map locals;
       local_map tmps;
       exit_point_list exits;
       dtor_list dtors_after_ins;
       string next_comment;

       // Stack of current catch clauses. When an exception occurs we jump to
       // top-most, which either handles it or forwards to the one just
       // beneath it. If not can handles, the last one unwinds further. 
       builder_list catches;
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
template<typename Result=int, typename Arg1=int, typename Arg2=int>
class CGVisitor : public hilti::Visitor<Result, Arg1, Arg2>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator the visitor is used by.
   ///
   /// logger_name: An identifying name passed on the Logger framework.
   CGVisitor(CodeGen* cg, const string& logger_name) {
       _codegen = cg;
       this->forwardLoggingTo(cg);
       this->setLoggerName(logger_name);
   }

   /// Returns the code generator this visitor is attached to.
   CodeGen* cg() const { return _codegen; }

   /// Returns the current builder, retrieved from the code generator. This
   /// is just a shortcut for calling CodeGen::builder().
   ///
   /// Returns: The current LLVM builder.
   IRBuilder* builder() const {
       return _codegen->builder();
   }

private:
   CodeGen* _codegen;
};


}
}

#endif
