
#ifndef BINPAC_CODEGEN_TYPE_BUILDER_H
#define BINPAC_CODEGEN_TYPE_BUILDER_H

#include "../common.h"
#include "cg-visitor.h"

namespace binpac {
namespace codegen {

// Codegen information about a BinPAC++ type.
struct TypeInfo {
    typedef std::function<shared_ptr<hilti::Expression> (CodeGen* cg, shared_ptr<binpac::Type> t)> expression_callback;

    ///  // The corresponding HILTI type. If null, one can't create instances
    ///  of this type.
    shared_ptr<hilti::Type>       hilti_type = nullptr;

    /// The default HILTI value for instances of this type that aren't
    /// explicitly initialized. If null, we use HILTI's default for the type.
    shared_ptr<hilti::Expression> hilti_default = nullptr;

    /// If true, instances of this value will always be intialized with \a
    /// hilti_default even if we could leave an instance unset (like in a
    /// union). That's typically useful for containters.
    bool always_initialize = false;
    };

/// Visitor that returns the HILTI type that corresponds to a BinPAC type.
class TypeBuilder : public CGVisitor<TypeInfo>
{
public:
    typedef std::list<shared_ptr<::hilti::ID>> id_list;

    /// Constructor.
    ///
    /// cg: The code generator to use. This may be left null if one just
    /// wants to call hiltiType() outside of code generation (but not other
    /// methods).
    TypeBuilder(CodeGen* cg);
    virtual ~TypeBuilder();

    /// Converts a BinPAC type into its corresponding HILTI type.
    ///
    /// type: The type to convert.
    ///
    /// deps: If given, any types that will need to be imported will be
    /// added to the list with their qualified IDs.
    ///
    /// Returns: The HILTI type, or null if not defined.
    shared_ptr<hilti::Type> hiltiType(shared_ptr<Type> type, id_list* deps = nullptr);

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

protected:
    void visit(type::Address* a) override;
    void visit(type::Any* a) override;
    void visit(type::Bitset* b) override;
    void visit(type::Block* b) override;
    void visit(type::Bool* b) override;
    void visit(type::Bytes* b) override;
    void visit(type::CAddr* c) override;
    void visit(type::Double* d) override;
    void visit(type::Enum* e) override;
    void visit(type::EmbeddedObject* e) override;
    void visit(type::Exception* e) override;
    void visit(type::File* f) override;
    void visit(type::Function* f) override;
    void visit(type::Hook* h) override;
    void visit(type::Integer* i) override;
    void visit(type::Interval* i) override;
    void visit(type::Iterator* i) override;
    void visit(type::List* l) override;
    void visit(type::Map* m) override;
    void visit(type::Module* m) override;
    void visit(type::Network* n) override;
    void visit(type::OptionalArgument* o) override;
    void visit(type::Port* p) override;
    void visit(type::RegExp* r) override;
    void visit(type::Set* s) override;
    void visit(type::Sink* s) override;
    void visit(type::String* s) override;
    void visit(type::Time* t) override;
    void visit(type::Tuple* t) override;
    void visit(type::TypeByName* t) override;
    void visit(type::TypeType* t) override;
    void visit(type::Unit* u) override;
    void visit(type::Unknown* u) override;
    void visit(type::Unset* u) override;
    void visit(type::Vector* v) override;
    void visit(type::Void* v) override;
    void visit(type::function::Parameter* p) override;
    void visit(type::function::Result* r) override;
    void visit(type::iterator::Bytes* b) override;
    void visit(type::iterator::List* l) override;
    void visit(type::iterator::Regexp* r) override;
    void visit(type::iterator::Set* s) override;
    void visit(type::iterator::Vector* v) override;
    void visit(type::unit::Item* i) override;
    void visit(type::unit::item::Field* f) override;
    void visit(type::unit::item::GlobalHook* g) override;
    void visit(type::unit::item::Property* p) override;
    void visit(type::unit::item::Variable* v) override;
    void visit(type::unit::item::field::Constant* c) override;
    void visit(type::unit::item::field::Ctor* c) override;
    void visit(type::unit::item::field::Switch* s) override;
    void visit(type::unit::item::field::AtomicType* t) override;
    void visit(type::unit::item::field::Unit* t) override;
    void visit(type::unit::item::field::switch_::Case* c) override;

private:
    id_list* _deps;

};

}
}

#endif
