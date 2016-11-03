/**
 * Spicy visitor that extracts information from an AST that we need later
 * for compiling.
 */

#ifndef BRO_PLUGIN_HILTI_SPICYAST_H
#define BRO_PLUGIN_HILTI_SPICYAST_H

#include <spicy/spicy.h>

namespace bro {
namespace hilti {

struct SpicyModuleInfo;

/// That visitor that provides information extracted from an Spicy AST.
class SpicyAST : ast::Visitor<spicy::AstInfo> {
public:
    /// A struct recording information about a unit type found in a *.spicy
    /// file.
    struct UnitInfo {
        string name;                              // The fully-qualified name of the unit type.
        bool exported;                            // True if the unit is exported.
        shared_ptr<spicy::type::Unit> unit_type; // The unit's type.
        shared_ptr<SpicyModuleInfo> minfo;         // The module this unit was defined in.
    };

    typedef std::map<string, shared_ptr<UnitInfo>> unit_map;

    SpicyAST()
    {
    }

    /**
     * Walks the AST of one module. The walker accumulates information
     * from all ASTs it has walked already, and makes the information
     * available through the corresponding accessor methods.
     */
    void process(shared_ptr<SpicyModuleInfo> minfo, shared_ptr<spicy::Module> module);

    /**
     * Looks up a fully-qualified unit ID and returns the unit type if
     * found, or null if not.
     */
    shared_ptr<UnitInfo> LookupUnit(const string& id);

    /**
     * Returns a map of all units encountered so far, indexed by their
     * fully-qualified unit names.
     */
    const unit_map& Units() const
    {
        return units;
    }

private:
    void visit(spicy::Module* m) override;
    void visit(spicy::declaration::Type* t) override;

    unit_map units;
    shared_ptr<SpicyModuleInfo> minfo;
};

#endif
}
}
