
#ifndef BINPAC_PASSES_GRAMMAR_BUILDER_H
#define BINPAC_PASSES_GRAMMAR_BUILDER_H

#include <map>

#include <ast/pass.h>

#include "common.h"
#include "ast-info.h"

namespace binpac {
namespace passes {

/// Compiles the grammars for all unit types.
class GrammarBuilder : public ast::Pass<AstInfo, shared_ptr<Production>>
{
public:
    /// Constructir.
    ///
    /// out: Stream where debug output is to go, if enabled.
    GrammarBuilder(std::ostream& out);
    virtual ~GrammarBuilder();

    /// Enables debug output, printing out all generated grammars.
    void enableDebug();

    bool run(shared_ptr<ast::NodeBase> ast) override;

protected:
    shared_ptr<Production> compileOne(shared_ptr<Node> n);
    string counter(const string& key);

    void visit(declaration::Type* d) override;
    void visit(type::Unit* u) override;
    void visit(type::unit::item::field::Constant* c) override;
    void visit(type::unit::item::field::RegExp* r) override;
    void visit(type::unit::item::field::Switch* s) override;
    void visit(type::unit::item::field::Type* t) override;
    void visit(type::unit::item::field::switch_::Case* c) override;

private:
    bool _debug = false;
    std::ostream& _debug_out;
    bool _in_decl;
    int _unit_counter = 1;
    std::map<string, int> _counters;
};

}
}

#endif
