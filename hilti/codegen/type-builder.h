
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

    /// For DynamicTypes, a pointer map for garbage collection. Can be null if
    /// instances don't store pointers.
    llvm::Constant* ptr_map = nullptr;

    /// For ValueTypes, the constant to initialize variables with. This value
    /// is also used to determine the number of bytes the instances of the
    /// type require for the storage.
    llvm::Constant* init_val = nullptr;

    /// For ValueTypes, true if the the HILTI-C calling convention passes type
   /// information for this type.
    bool pass_type_info = false;

    /// The C prototype for the type to use in header files.
    string c_prototype = "<no prototype>";

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

    /// For garbage collected objects, the name of an internal libhilti
    /// function that will be called when an object's reference count went
    /// down to zero and its about to be deleted. The function receives a
    /// pointer to the object as its single parameter. Can be left unset if a
    /// type does not need any further cleanup.
    string dtor = "";

    /// A global LLVM variable with auxiliary type-specific RTTI information,
    /// which will be accessible from the C level.
    llvm::Constant *aux = nullptr;
};

/// Visitor class that generates type information for a type. Note that this
/// class should not be used directly, the main frontend functions are
/// CodeGen::llvmRtti(), CodeGen::llvmType, CodeGen::typeInfo().
class TypeBuilder : public CGVisitor<TypeInfo*>
{
public:
   /// Constructor.
   ///
   /// cg: The code generator to use.
   TypeBuilder(CodeGen* cg);
   virtual ~TypeBuilder();

   /// Returns the type information structure for a type.
   ///
   /// type: The type to return the information for.
   ///
   /// Returns: The object with the type's information.
   shared_ptr<TypeInfo> typeInfo(shared_ptr<hilti::Type> type);

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

protected:
// void visit(type::Function* f) override;
   void visit(type::Any* a) override;
   void visit(type::Void* v) override;
   void visit(type::String* s) override;
   void visit(type::Integer* u) override;
   void visit(type::Bool* b) override;
   void visit(type::Tuple* t) override;

private:
   llvm::Constant* _lookupFunction(const string& func);

   // We cached all once computed type-infos and return the cached version
   // next time somebody asks for the same type instance.
   typedef std::map<shared_ptr<hilti::Type>, shared_ptr<TypeInfo>> ti_cache_map;
   ti_cache_map _ti_cache;
};


}
}

#endif
