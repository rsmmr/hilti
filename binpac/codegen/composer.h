
#ifndef BINPAC_CODEGEN_COMPOSER_H
#define BINPAC_CODEGEN_COMPOSER_H

#include <ast/visitor.h>

#include "../common.h"
#include "../type.h"
#include "cg-visitor.h"

namespace binpac {

namespace type {
namespace unit {
namespace item {
class Field;
}
}
}

namespace codegen {

class ComposerState;

/// Generates code to compose input according to a grammar.
class Composer : public codegen::CGVisitor<shared_ptr<hilti::Expression>,
                                           shared_ptr<type::unit::item::Field>> {
public:
    Composer(CodeGen* cg);
    virtual ~Composer();

    /// Returns the type of the currently composed unit. The method must only
    /// be called when composing is in progress.
    shared_ptr<type::Unit> unit() const;

    /// Returns the currently composed value. The method must only
    /// be called when composing is in progress.
    shared_ptr<hilti::Expression> hiltiObject(
        shared_ptr<type::unit::item::Field> field = nullptr) const;

    // Creates the host-facing composer function.
    //
    // unit: The unit to create the host function for.
    shared_ptr<hilti::Expression> hiltiCreateHostFunction(shared_ptr<type::Unit> unit);

    /// Generates the function to compose input according to a unit's
    /// grammar.
    ///
    /// u: The unit to generate the composer for.
    ///
    /// Returns: The generated HILTI function with the composing code.
    shared_ptr<hilti::Expression> hiltiCreateComposeFunction(shared_ptr<type::Unit> u);

    /// Generates the externally visible functions for composing a unit type.
    ///
    /// u: The unit type to export via functions.
    void hiltiExportComposer(shared_ptr<type::Unit> unit);

    /// Generates the implementation of unit-embedded compose hooks.
    ///
    /// u: The unit type to generate the hooks for.
    void hiltiUnitHooks(shared_ptr<type::Unit> unit);

    /// Adds an external implementation of a unit compose hook.
    ///
    /// id: The hook's ID (full path).
    ///
    /// hook: The hook itself.
    void hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook);

    /// Generates code to execute the hooks associated with an unit item.
    /// This must only be called while a unit is being composed.
    ///
    /// item: The item.
    ///
    /// self: The expression to pass as the hook's \a self argument. Must
    /// match the type of the unit that \a item is part of.
    void hiltiRunFieldHooks(shared_ptr<type::unit::Item> item, shared_ptr<hilti::Expression> self);

protected:
    /// Compose a given entity. This a wrapper around processOne() that adds
    /// additional logic around it.
    void compose(shared_ptr<Node> node, shared_ptr<type::unit::item::Field> field = nullptr);

    /// Compose a given entity. This verson accepts an explicit pointer to
    /// the value to compose.
    void compose(shared_ptr<Node> node, shared_ptr<hilti::Expression> obj,
                 shared_ptr<type::unit::item::Field> field = nullptr);

    /// Returns the current composing state.
    shared_ptr<ComposerState> state() const;

    /// Pushes a new composing state onto the stack.
    void pushState(shared_ptr<ComposerState> state);

    /// Pops the current composing state from the stack.
    void popState();

    void visit(production::Boolean* b) override;
    void visit(production::ChildGrammar* c) override;
    void visit(production::Enclosure* e) override;
    void visit(production::Counter* c) override;
    void visit(production::ByteBlock* c) override;
    void visit(production::Epsilon* e) override;
    void visit(production::Literal* l) override;
    void visit(production::LookAhead* l) override;
    void visit(production::Sequence* s) override;
    void visit(production::Switch* s) override;
    void visit(production::Variable* v) override;
    void visit(production::Loop* l) override;

    void visit(constant::Integer* i) override;

    void visit(ctor::Bytes* b) override;
    void visit(ctor::RegExp* r) override;

    void visit(expression::Ctor* c) override;
    void visit(expression::Constant* c) override;
    void visit(expression::Type* c) override;

    void visit(type::Bytes* b) override;
    void visit(type::Integer* i) override;
    void visit(type::Address* a) override;
    void visit(type::Bitfield* b) override;

#if 0
    void visit(constant::Address* a) override;
    void visit(constant::Bitset* b) override;
    void visit(constant::Bool* b) override;
    void visit(constant::Double* d) override;
    void visit(constant::Enum* e) override;
    void visit(constant::Interval* i) override;
    void visit(constant::Network* n) override;
    void visit(constant::Port* p) override;
    void visit(constant::String* s) override;
    void visit(constant::Time* t) override;

    void visit(type::Bitset* b) override;
    void visit(type::Bool* b) override;
    void visit(type::Double* d) override;
    void visit(type::Enum* e) override;
    void visit(type::EmbeddedObject* o) override;
    void visit(type::Interval* i) override;
    void visit(type::List* l) override;
    void visit(type::Network* n) override;
    void visit(type::Port* p) override;
    void visit(type::Sink* s) override;
    void visit(type::String* s) override;
    void visit(type::Set* s) override;
    void visit(type::Time* t) override;
    void visit(type::Unit* u) override;
    void visit(type::Void* v) override;
    void visit(type::unit::Item* i) override;
    void visit(type::unit::item::Field* f) override;
    void visit(type::unit::item::field::Constant* c) override;
    void visit(type::unit::item::field::Ctor* r) override;
    void visit(type::unit::item::field::Switch* s) override;
    void visit(type::unit::item::field::AtomicType* t) override;
    void visit(type::unit::item::field::Unit* t) override;
    void visit(type::unit::item::field::switch_::Case* c) override;
    void visit(type::unit::item::Variable* v) override;
    void visit(type::unit::item::Property* p) override;
    void visit(type::unit::item::GlobalHook* h) override;
    void visit(type::Vector* v) override;
#endif

private:
    typedef std::list<std::pair<shared_ptr<hilti::Expression>, shared_ptr<Type>>>
        hilti_expression_type_list;

    // Pushes an empty compose function with the right standard signature. If
    // value_type is given, the function return tuple will contain an
    // additional element of that type for passing back the composed value.
    //
    // Returns: An expression referencing the function.
    shared_ptr<hilti::Expression> _newComposeFunction(const string& name,
                                                      shared_ptr<type::Unit> unit,
                                                      shared_ptr<hilti::Type> value_type = nullptr);

    // Finishes a functions started with _newcomposeFunction().
    void _finishComposeFunction();

    /// Generates the body code for composing a given node. Same parameters as
    /// compose().
    void _hiltiCompose(shared_ptr<Node> node, shared_ptr<hilti::Expression> obj,
                       shared_ptr<type::unit::item::Field> f);

    /// Composes a container value.
    void _hiltiComposeContainer(shared_ptr<hilti::Expression> value, shared_ptr<Production> body,
                                shared_ptr<type::unit::item::field::Container> container);

    // Called just before a production is being composed.
    void _startingProduction(shared_ptr<Production> p, shared_ptr<type::unit::item::Field> field);

    // Called juest before starting to compose a unit.
    void _startingUnit();

    // Called just after composing a unit.
    void _finishedUnit();

    // Called just after a production has been composed.
    void _finishedProduction(shared_ptr<Production> p);

    /// Generates a HILTI ``pack`` instruction and passes the data on to the
    /// host application.
    ///
    /// field: The field associated with the data.
    ///
    /// op1, op2, op3: Same as with the regular ``pack`` instruction.
    void hiltiPack(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> op1,
                   shared_ptr<hilti::Expression> op2, shared_ptr<hilti::Expression> op3 = nullptr);

    /// Generates code to pass a chunk of composed data to host application.
    /// The data will be associated with the current position in the compose
    /// process.
    void _hiltiDataComposed(shared_ptr<hilti::Expression> data,
                            shared_ptr<type::unit::item::Field> field);

    // Calls a compose function with the current composing state.
    void _hiltiCallComposeFunction(shared_ptr<binpac::type::Unit> unit,
                                   shared_ptr<hilti::Expression> func);

    // Run a field's hooks.
    void _hiltiRunFieldHooks(shared_ptr<type::unit::item::Field> field);

    // Prints the given message to the binpac-compose debug stream.
    void _hiltiDebug(const string& msg);

    // Prints the given message to the binpac-compose-verbose debug stream.
    void _hiltiDebugVerbose(const string& msg);

    // Raises a ComposeError exception.
    void _hiltiComposeError(const string& msg);

    // Raises a ComposeError exception given an existing exception.
    void _hiltiComposeError(shared_ptr<hilti::Expression> excpt);

    // Helper to transparently pipe input through a chain of filters if a
    // composing object has defined any. It adjust the composing arguments
    // appropiately so that subsequent code doesn't notice the filtering.
    //
    // resume: True if this method is called after resuming composing because
    // of having insufficient inout earlier. False if it's the initial
    // composing step for a newly instantiated composer.
    void _hiltiFilterOutput();

    std::list<shared_ptr<ComposerState>> _states;
    shared_ptr<hilti::Expression> _object;
};
}
}

#endif
