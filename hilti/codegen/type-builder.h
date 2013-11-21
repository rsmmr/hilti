
#ifndef HILTI_CODEGEN_TYPE_BUILDER_H
#define HILTI_CODEGEN_TYPE_BUILDER_H

#include "../common.h"
#include "../visitor.h"

#include "common.h"
#include "codegen.h"

#include "../../libhilti/rtti.h"

namespace hilti {

namespace codegen {

/// Class that build a pointer-map for the HILTI run-times garbage collector.
///
/// Note that currently these pointer-maps are generated, but not used yet.
///
/// Also, currently it supports only types that fit one of the constructors.
/// More will be added as we get more types.
class PointerMap
{
public:
   /// Constructor for types of trait type::trait::TypeList. The created
   /// pointer map assume that all fields are just linearly stored in a
   /// not-packed struct in the order as returned by
   /// type::trait::TypeList::typeList(). If the type is
   /// type::trait::GarbabeCollected, we further assume that it begins with
   /// the standard ``hlt.ref_cnt`` header.
   ///
   /// cg: The code generator to use.
   ///
   /// type: The type to generate a pointer map for, which must be a
   /// type::trait::TypeList.
   PointerMap(CodeGen* cg, hilti::Type* type);

   /// Returns the computed pointer map. It can be directly stored in the
   /// corresponding TypeInfo instance.
   llvm::Constant* llvmMap();

private:
   CodeGen* _cg;
   std::vector<llvm::Constant*> _offsets;
};

/// Type information describing properties of HILTI's types.
///
/// The fields in this struct are all public and can be accessed directly.
struct TypeInfo {
    /// Constructor.
    ///
    /// t: The described type.
    TypeInfo(shared_ptr<hilti::Type> t) {
        type = t;
    }

    /// Constructor.
    ///
    /// t: The described type.
    TypeInfo(hilti::Type* t) {
        type = t->sharedPtr<hilti::Type>();
    }

    /// The type the information record describes. Set by ctor.
    shared_ptr<Type> type = nullptr;

    /// The RTTI type ID, defined in libhilti/rtti.h.
    int16_t id = HLT_TYPE_ERROR;

    /// A string representation of the type suitable to showing in messages to
    /// the user. Will be set to Type::repr() if not given.
    string name = "";

    /// For HiltiTypes, the corresponding LLVM type. Can be left unset if
    /// ``init_val`` is defined.
    llvm::Type* llvm_type = nullptr;

    /// The name of a library type to use. This is can be given instead of \a
    /// llvm_type. With reference types, a \a pointer to this type will then
    /// be used.
    string lib_type = "";

    /// For DynamicTypes, a pointer map for garbage collection. Can be null if
    /// instances don't store pointers.
    llvm::Constant* ptr_map = nullptr;

    /// For ValueTypes, the constant to initialize variables with. This value
    /// is also used to determine the number of bytes the instances of the
    /// type require for the storage. If llvm_type or lib_type is set, this
    /// defaults to a null value of that type.
    llvm::Constant* init_val = nullptr;

    /// For ValueTypes, true if the the HILTI-C calling convention passes type
   /// information for this type.
    bool pass_type_info = false;

    /// The name of an internal libhilti function that converts a value of the
    /// type into a readable string representation. See ``hilti-intern.h`` for
    /// for the function's complete C signature. Empty string if not
    /// applicable.
    string to_string = "";

    /// The name of an internal libhilti function that converts a value of the
    /// type into an int<64>. See ``hilti-intern.h`` for for the function's
    /// complete C signature. Empty string if not applicable.
    string to_int64 = "";

    /// The name of an internal libhilti function that converts a value of the
    /// type into a double. See ``hilti-intern.h`` for for the function's
    /// complete C signature. Empty string if not applicable.
    string to_double = "";

    /// The name of an internal libhilti function that calculates a hash value
    /// for an instance of the type. See ``hilti-intern.h`` for the function's
    /// complete C signature. Empty string if not comparable.
    string hash = "";

    /// The name of an internal libhilti function that compares two instances
    /// of the type. See ``hilti-intern.h`` for the function's complete C
    /// signature. Empty string if not comparable.
    string equal = "";

    /// The name of an internal libhilti function that returns an internal
    /// HILTI "blockable" object if a ``yield`` can wait for instances of this
    /// type to become available.  See ``hilti-intern.h`` for the function's
    /// complete C signature. Empty string if not supported.  If this field is
    /// set, the corresponding type must be derived from trait ~~Blockable.
    string blockable = "";

    /// The name of an internal libhilti function that will be called if an
    /// instance is going to be destroyed.It receives two parameters: type
    /// information for the instance and and pointer to the instance.
    string dtor = "";

    /// Like \a dtor, but giving the function (of the same signature)
    /// directly. Only one of \a dtor and \a dtor_func must be given.
    llvm::Function* dtor_func = 0;

    /// XXX
    string obj_dtor = "";

    /// XXX
    llvm::Function* obj_dtor_func = 0;

    /// The name of an internal libhilti function that will be called if an
    /// instance has been copied. The function is called after a bitwise copy
    /// has been made, but before any further operation is executed that
    /// involves it. It receives two parameters: type information for the
    /// instance and and pointer to the instance.
    string cctor = "";

    /// Like \a dtor, but giving the function (of the same signature)
    /// directly. Only one of \a cctor and \a cctor_func must be given.
    llvm::Function* cctor_func = 0;

    /// A global LLVM variable with auxiliary type-specific RTTI information,
    /// which will be accessible from the C level.
    llvm::Constant *aux = nullptr;
};

/// Visitor class that generates type information for a type. Note that this
/// class should not be used directly, the main frontend functions are
/// CodeGen::llvmRtti(), CodeGen::llvmType, CodeGen::typeInfo().
///
/// The boolean parameter is internal and if true indicate that we want to
/// retrieve only llvm_type field; others may be left unset (except that
/// init_val may be set instead of llvm_type) This helps dealing with cyclic
/// struct types.
class TypeBuilder : public CGVisitor<TypeInfo*, bool>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   TypeBuilder(CodeGen* cg);
   virtual ~TypeBuilder();

   /// Finalizes type building. This method must be called at the very end of
   /// code generation.
   void finalize();

   /// Returns the type information structure for a type.
   ///
   /// type: The type to return the information for.
   ///
   /// Returns: The object with the type's information.
   TypeInfo* typeInfo(shared_ptr<hilti::Type> type) {
       return typeInfo(type, false);
   }

   /// Returns the LLVM type that corresponds to a HILTI type.
   ///
   /// type: The type to return the LLVM type for.
   ///
   /// Returns: The corresponding LLVM type.
   llvm::Type* llvmType(shared_ptr<hilti::Type> type);

   /// Returns the LLVM RTTI object for a HILTI type.
   ///
   /// type: The type to return the RTTI for.
   ///
   /// Returns: The corresponding LLVM value.
   llvm::Constant* llvmRtti(shared_ptr<hilti::Type> type);

   /// Returns the concretet LLVM type for a type's RTTI object. Note that
   /// this is not the generic \a hlt_type_info but the specific type for
   /// this HILTi type.
   ///
   /// type: The type to return the RTTI for.
   ///
   /// Returns: The corresponding LLVM value.
   llvm::Type* llvmRttiType(shared_ptr<hilti::Type> type);

protected:
// void visit(type::Function* f) override;
   void visit(type::Address* t) override;
   void visit(type::Any* t) override;
   void visit(type::Bitset* t) override;
   void visit(type::Bool* t) override;
   void visit(type::Bytes* t) override;
   void visit(type::CAddr* t) override;
   void visit(type::Callable* t) override;
   void visit(type::Channel* t) override;
   void visit(type::Classifier* t) override;
   void visit(type::Double* t) override;
   void visit(type::Enum* t) override;
   void visit(type::Exception* t) override;
   void visit(type::File* t) override;
   void visit(type::Function* t) override;
   void visit(type::IOSource* t) override;
   void visit(type::Integer* t) override;
   void visit(type::Interval* t) override;
   void visit(type::List* t) override;
   void visit(type::Map* t) override;
   void visit(type::MatchTokenState* t) override;
   void visit(type::Network* t) override;
   void visit(type::Overlay* t) override;
   void visit(type::Port* t) override;
   void visit(type::Reference* t) override;
   void visit(type::RegExp* t) override;
   void visit(type::Set* t) override;
   void visit(type::String* t) override;
   void visit(type::Struct* t) override;
   void visit(type::Time* t) override;
   void visit(type::Timer* t) override;
   void visit(type::TimerMgr* t) override;
   void visit(type::Tuple* t) override;
   void visit(type::Unset* t) override;
   void visit(type::Vector* t) override;
   void visit(type::Void* t) override;

   void visit(type::iterator::Bytes* i) override;
   void visit(type::iterator::List* i) override;
   void visit(type::iterator::Map* i) override;
   void visit(type::iterator::Set* i) override;
   void visit(type::iterator::Vector* i) override;
   void visit(type::iterator::IOSource* i) override;

private:
   TypeInfo* typeInfo(shared_ptr<hilti::Type> type, bool llvm_type_only);

   llvm::Constant* _lookupFunction(const string& func);
   llvm::Function* _makeTupleDtor(CodeGen* cg, type::Tuple* type);
   llvm::Function* _makeTupleCctor(CodeGen* cg, type::Tuple* type);
   llvm::Function* _makeTupleFuncHelper(CodeGen* cg, type::Tuple* t, bool dtor);
   llvm::Function* _makeOverlayCctor(CodeGen* cg, type::Overlay* t, llvm::Type* llvm_type);
   llvm::Function* _makeOverlayDtor(CodeGen* cg, type::Overlay* t, llvm::Type* llvm_type);
   llvm::Function* _makeOverlayFuncHelper(CodeGen* cg, type::Overlay* t, llvm::Type* llvm_type, bool dtor);

   llvm::Function* _declareStructDtor(type::Struct* t, llvm::Type* llvm_type);
   void _makeStructDtor(type::Struct* t, llvm::Function* func);

   // We cached all once computed type-infos and return the cached version
   // next time somebody asks for the same type instance.
   typedef std::map<shared_ptr<hilti::Type>, TypeInfo*> ti_cache_map;
   ti_cache_map _ti_cache;

   // We cache structs by name so that we always map them to the same type.
   std::map<string, llvm::StructType*> _known_structs;

   // We acculumate struct dtor functions first and then build them at the end.
   std::list<std::pair<type::Struct*, llvm::Function*>> _struct_dtors;

   bool _rtti_type_only = false;

};


}
}

#endif
