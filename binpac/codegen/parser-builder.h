
#ifndef BINPAC_CODEGEN_PARSER_BUILDER_H
#define BINPAC_CODEGEN_PARSER_BUILDER_H

#include <ast/visitor.h>

#include "common.h"
#include "cg-visitor.h"

namespace binpac {
namespace codegen {

class ParserState;

/// Generates code to parse input according to a grammar.
class ParserBuilder : public CGVisitor<shared_ptr<hilti::Expression>>
{
public:
    ParserBuilder(CodeGen* cg);
    virtual ~ParserBuilder();

    /// Generates the function to parse input according to a unit's grammar.
    ///
    /// u: The unit to generate the parser for.
    ///
    /// Returns: The generated HILTI function with the parsing code.
    shared_ptr<hilti::Expression> hiltiCreateParseFunction(shared_ptr<type::Unit> u);

    /// Generates the externally visible functions for parsing a unit type.
    ///
    /// u: The unit type to export via functions.
    void hiltiExportParser(shared_ptr<type::Unit> unit);

    // Returns the HILTI struct type for a unit's parse object.
    shared_ptr<hilti::Type> hiltiTypeParseObject(shared_ptr<type::Unit> unit);

protected:
    /// Returns the current parsing state.
    shared_ptr<ParserState> state() const;

    /// Pushes a new parsing state onto the stack.
    void pushState(shared_ptr<ParserState> state);

    /// Pops the current parsing state from the stack.
    void popState();

    void visit(constant::Address* a) override;
    void visit(constant::Bitset* b) override;
    void visit(constant::Bool* b) override;
    void visit(constant::Double* d) override;
    void visit(constant::Enum* e) override;
    void visit(constant::Integer* i) override;
    void visit(constant::Interval* i) override;
    void visit(constant::Network* n) override;
    void visit(constant::Port* p) override;
    void visit(constant::String* s) override;
    void visit(constant::Time* t) override;

    void visit(ctor::Bytes* b) override;
    void visit(ctor::RegExp* r) override;

    void visit(production::Boolean* b) override;
    void visit(production::ChildGrammar* c) override;
    void visit(production::Counter* c) override;
    void visit(production::Epsilon* e) override;
    void visit(production::Literal* l) override;
    void visit(production::LookAhead* l) override;
    void visit(production::NonTerminal* n) override;
    void visit(production::Sequence* s) override;
    void visit(production::Switch* s) override;
    void visit(production::Terminal* t) override;
    void visit(production::Variable* v) override;
    void visit(production::While* w) override;

    void visit(type::Address* a) override;
    void visit(type::Bitset* b) override;
    void visit(type::Bool* b) override;
    void visit(type::Bytes* b) override;
    void visit(type::Double* d) override;
    void visit(type::Enum* e) override;
    void visit(type::Integer* i) override;
    void visit(type::Interval* i) override;
    void visit(type::List* l) override;
    void visit(type::Network* n) override;
    void visit(type::Port* p) override;
    void visit(type::Set* s) override;
    void visit(type::String* s) override;
    void visit(type::Time* t) override;
    void visit(type::Unit* u) override;
    void visit(type::unit::item::field::Constant* c) override;
    void visit(type::unit::item::field::RegExp* r) override;
    void visit(type::unit::item::field::Switch* s) override;
    void visit(type::unit::item::field::Type* t) override;
    void visit(type::unit::item::field::switch_::Case* c) override;
    void visit(type::Vector* v) override;

private:
    // Pushes an empty parse function with the right standard signature.
    //
    // Returns: An expression referencing the function.
    shared_ptr<hilti::Expression> _newParseFunction(const string& name, shared_ptr<type::Unit> unit);

    // Allocates and initializes a new parse object.
    shared_ptr<hilti::Expression> _allocateParseObject(shared_ptr<Type> unit, bool store_in_self);

    // Initializes the current parse object before starting the parsing
    // process.
    void _prepareParseObject();

    // Finalizes the current parser when the parsing process has finished.
    void _finalizeParseObject();

    // Called just before a production is being parsed.
    void _startingProduction(shared_ptr<Production> p);

    // Called just after a production has been parsed.
    void _finishedProduction(shared_ptr<Production> p);

    // Callen when parsing a production leads to a new value to be assigned
    // to a user-visible field of the current parse object. This method is
    // doing the actual assignment.
    void _newValueForField(shared_ptr<Production> prod, shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> value);

    // Creates the host-facing parser function. If sink is true, we generate
    // a slightly different version for internal use with sinks.
    shared_ptr<hilti::Expression> _hiltiCreateHostFunction(shared_ptr<type::Unit> unit, bool sink);

    // Creates the init function that registers a parser with the binpac runtime.
    void _hiltiCreateParserInitFunction(shared_ptr<type::Unit> unit, shared_ptr<hilti::Expression> parse_host, shared_ptr<hilti::Expression> parse_sink);

    // Returns the BinPAC::Parser instance for a unit.
    shared_ptr<hilti::Expression> _hiltiParserDefinition(shared_ptr<type::Unit> unit);

    // Prints the given message to the binpac debug stream.
    void _hiltiDebug(const string& msg);

    // Prints the given message to the binpac verbose debug stream.
    void _hiltiDebugVerbose(const string& msg);

    // Prints the given token to binpac-verbose.
    void _hiltiDebugShowToken(const string& tag, shared_ptr<hilti::Expression> token);

    /// Prints the upcoming input bytes to binpac-verbose.
    void _hiltiDebugShowInput(const string& tag, shared_ptr<hilti::Expression> cur);

    std::list<shared_ptr<ParserState>> _states;
};

}
}

#endif
