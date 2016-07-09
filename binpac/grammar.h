///
/// A grammar to generate a BinPAC++ parser from.
///

#ifndef BINPAC_PGEN_GRAMMAR_H
#define BINPAC_PGEN_GRAMMAR_H

#include <map>

#include "common.h"
#include "production.h"

namespace binpac {

/// A BinPAC++ grammar.
class Grammar {
public:
    typedef std::list<std::pair<shared_ptr<ID>, shared_ptr<Type>>> parameter_list;
    typedef std::list<shared_ptr<production::NonTerminal>> nterm_list;
    typedef std::map<string, shared_ptr<Production>> production_map;

    /// Constructor.
    ///
    /// name: A name associated with the grammar, mainly for debugging
    /// purposes but also used for generating labels during code generation.
    ///
    /// root: The top-level root production.
    ///
    /// params: Parameters passed into the grammar.
    ///
    /// attrs: Additional attributes that will become part of the generated
    /// parsed object as further fields.
    ///
    /// l: Associated location.
    ///
    /// \todo: I don't think we need attrs and params anymore?
    Grammar(const string& name, shared_ptr<Production> root,
            const parameter_list& params = parameter_list(),
            const parameter_list& attrs = parameter_list(), const Location& l = Location::None);

    /// Returns the name of the grammar. The name uniquely identifies the
    /// grammar.
    const string& name() const;

    /// Returns the location associated with the production, or Location::None
    /// if none.
    const Location& location() const;

    /// Returns the parameters passed into the grammar' parser.
    const parameter_list& parameters() const;

    /// Checks the grammar for ambiguity. From an ambigious grammar, no parser
    /// can be generated.
    ///
    /// Returns: If the return value is an empty string, the grammar is fine;
    /// otherwise the returned string contains a description of the
    /// ambiguities encountered, suitable for inclusion in an error message.
    string check() const;

    /// Returns the grammar's productions. The method returns a map that maps
    /// each production's symbol to the Production itself. Production's
    /// without symbols are not included.
    const production_map& productions() const;

    /// Returns the root production.
    shared_ptr<Production> root() const;

    /// Returns true if the grammar needs look-ahead for parsing.
    bool needsLookAhead() const;

    /// Prints the grammar in a (somewhat) human readable form. This is for
    /// debugging. In *verbose* mode, the grammar and all the internal
    /// nullable/first/follow tables are printed.
    ///
    /// out: Where to print to.
    /// verbose: True for verbose output.
    void printTables(std::ostream& out, bool verbose = false);

private:
    typedef Production::production_list production_list;
    typedef std::set<shared_ptr<Production>> production_set;
    typedef std::set<string> symbol_set;

    void _addProduction(shared_ptr<Production> p);
    void _simplify();
    void _computeTables();

    void _computeClosure(shared_ptr<Production> root, std::set<string>* used);
    bool _add(std::map<string, symbol_set>* tbl, shared_ptr<Production> dst, const symbol_set& src,
              bool changed);
    bool _isNullable(production_list::iterator i, production_list::iterator j);
    symbol_set _getFirst(shared_ptr<Production> p);
    symbol_set _getFirstOfRhs(production_list rhs);
    string _productionLocation(shared_ptr<Production> p) const;

    string _name;
    bool _needs_look_ahead = false;
    parameter_list _params;
    parameter_list _attrs;
    Location _location;
    shared_ptr<Production> _epsilon;
    shared_ptr<Production> _root;

    production_map _prods;
    production_set _prods_set;
    nterm_list _nterms;

    production_list _lah_errors;

    std::map<string, bool> _nullable;
    std::map<string, symbol_set> _first;
    std::map<string, symbol_set> _follow;
};
}

#endif
