///
/// A grammar to generate a BinPAC++ parser from.
///

#ifndef BINPAC_GRAMMAR_H
#define BINPAC_GRAMMAR_H

#include "common.h"

namespace binpac {

/// A BinPAC++ grammar.
class Grammar
{
public:
    typedef std::list<std::pair<shared_ptr<ID>, shared_ptr<Type>>> parameter_list;

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
    Grammar(const string& name, shared_ptr<Production> root, const parameter_list& params = parameter_list(),
           const parameter_list& attrs = parameter_list(), const Location& l = Location::None);

    /// Returns the name of the grammar. The name uniquely identifies the
    /// grammar.
    const string& name() const { return _name; }

    /// Returns the location associated with the production, or Location::None
    /// if none.
    const Location& location() const { return _location; }

    /// Returns the parameters passed into the grammar' parser.
    const parameter_list& parameters() const { return _params; }

    /// Checks the grammar for ambiguity. From an ambigious grammar, no parser
    /// can be generated.
    ///
    /// Returns: If the return value is an empty string, the grammar is fine;
    /// otherwise the returned string contains a description of the
    /// ambiguities encountered, suitable for inclusion in an error message.
    string check() const;

    /// Returns the grammar's productions. The method returns a dictionary
    /// that maps each production's symbol to the Production itself.
    /// Production's without symbols are not included.
    std::map<string, shared_ptr<Production>> productions() const;

    /// Returns a scope for this grammar that contains all identifier defined
    /// by this grammar.
    shared_ptr<Scope> scope() const { return _scope; }

    /// Returns the starting production.
    shared_ptr<Production> start() const { self._start; }

    /// Prints the grammar in a (somewhat) human readable form. This is for
    /// debugging. In *verbose* mode, the grammar and all the internal
    /// nullable/first/follow tables are printed.
    ///
    /// out: Where to print to.
    /// verbose: True for verbose output.
    void printTables(self, std::ostream& out, bool verbose = false);

private:
    typedef Production::production_list production_list;

    void _addProduction(shared_ptr<Production> p, bool add_ids=True);
    void _simplify();
    void _computeTables();

    // void _add(tbl, dst, src, changed);
    // bool _nullable(rhs, i, j);
    // production_list _first(prod);
    // production_list _firstOfRhs(prod);

    string _name;
    shared_ptr<Production> _root;
    parameter_list _params;
    parameter_list _attrs;
    Location _location;
};

}

#endif
