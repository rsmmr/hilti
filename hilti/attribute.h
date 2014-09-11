
#ifndef HILTI_ATTRIBUTE_H
#define HILTI_ATTRIBUTE_H

#include <vector>

#include "ast-info.h"

namespace hilti {

namespace attribute {

/**
 * Available attributes. These include all \c &... attributes available
 * anywhere within the HILTI language.
 */
enum Tag {
    CANREMOVE,
    DEFAULT,
    FIRSTMATCH,
    GROUP,
    HOIST,
    LIBHILTI,
    LIBHILTI_DTOR,
    MAYYIELD,
    NOEXCEPTION,
    NOSAFEPOINT,
    NOSUB,
    NOYIELD,
    PRIORITY,
    REF,
    SAFEPOINT,
    SCOPE,

    // Marker for error case.
    ERROR,
};

/**
 * Context where an attribute can be used.
 */
enum Context {
    EXCEPTION,      /// Associated with exceptions.
    FUNCTION,       /// Associated with functions.
    MAP,            /// Associated with a map.
    REGEXP,         /// Associated with regexps.
    STRUCT,         /// Associated with structs.
    STRUCT_FIELD,   /// Associated with struct fields.
    VARIABLE,       /// Associated with variables.
    ANY,            /// Wildcard
};

}

class AttributeSet;

/**
 * Class to represent an single \c &attribute, with an optional value.
 */
class Attribute {
public:
    /**
     * Constructor for an attribute without value.
     *
     * tag: The attribute's tag.
     */
    Attribute(attribute::Tag tag = attribute::ERROR);

    /**
     * Constructor for an attribute with a value.
     *
     * tag: The attribute's tag.
     *
     * value: The value.
     */
    Attribute(attribute::Tag tag, node_ptr<Node> value);

    /**
     * Constructor for an attribute with an integer value.
     *
     * This is a short-cut that creates a creates a corresponding \c Node
     * object internally.
     *
     * tag: The attribute's tag.
     *
     * value: The value.
     */
    Attribute(attribute::Tag tag, int64_t value);

    /**
     * Constructor for an attribute with an integer value.
     *
     * This is a short-cut that creates a creates a corresponding \c Node
     * object internally.
     *
     * tag: The attribute's tag.
     *
     * value: The value.
     */
    Attribute(attribute::Tag tag, const std::string& value);

    /**
     * Returns the attributes tag.
     */
    attribute::Tag tag() const;

    /**
     * Returns true if there's a value associated with the attribute.
     */
    bool hasValue() const;

    /**
     * Returns the value associated with the attribute. Returns null if none.
     */
    shared_ptr<Node> value() const;

    /**
     * Validates an attribute. This checks (1) if the attribute is valid to
     * use in the given context, and (2) if it's value is speficied correctly.
     *
     * ctx: The context to validate the attribute in.
     *
     * msg: If given, will be set to an error message describing the problem
     * if validation fails.
     *
     * Returns true if correct, false otherwise.
     */
    bool validate(attribute::Context ctx, std::string* msg = nullptr) const;

    /**
     * Returns a textual representation of the attribute corresponding to how
     * the it would be specified in HILTI source code.
     */
    std::string render() const;

    /**
     * Returns true if the attribute's tag is not \c attribute::ERROR.
     */
    operator bool() const;

    /**
     * Returns the tag associated with a textual name. Returns an attribute
     * with tag \c attribute::ERROR if unknown.
     *
     * name: The name of convert. Can be given with or without a leading \c
     * &.
     */
    static attribute::Tag nameToTag(std::string name);

    /**
     * Returns the textual name associated with an attribute, excluding the
     * leading \c &.
     *
     * tag: The tag to convert.
     */
    static string tagToName(attribute::Tag tag);

private:
    friend class AttributeSet;

    attribute::Tag _tag;
    node_ptr<Node> _value;
};

/**
 * Groups a set of attributes into a set.
 */
class AttributeSet : public Node {
public:
    /**
     * Constructor.
     */
    AttributeSet();

    /**
     * Returns true if there's a attribute of the given tag in the set.
     *
     * tag: The tag to check.
     */
    bool has(attribute::Tag tag) const;

    /**
     * Retrieves the attribute matching a given tag from the set. If there's
     * no such attribute, returns one with tag \c attribute::ERROR.
     */
    Attribute get(attribute::Tag tag) const;

    /**
     * Retrieves the value for the attribute matching a given tag from the set.
     *
     * This is a convinience method that turns the node associated with the
     * attribute into an integer. It's a fatal error if the attribute doesn't
     * carry a constant integer value.
     *
     * tag: The tag to which to retrieve the value for.
     *
     * default_: A default value to return of no such attribute exists in the
     * set.
     *
     * Returns: The attributes value, or the default_.
     */
    int64_t getAsInt(attribute::Tag tag, int64_t default_) const;

    /**
     * Retrieves a string value for the attribute matching a given tag from
     * the set. If there's no such attribute, returns the specified default.
     *
     * This is a convinience method that turns the node associated with the
     * attribute into a string. It's a fatal error if the attribute doesn't
     * carry a constant string value.
     *
     * tag: The tag to which to retrieve the value for.
     *
     * default_: A default value to return of no such attribute exists in the
     * set.
     *
     * Returns: The attributes value, or the default_.
     */
    std::string getAsString(attribute::Tag tag, const std::string& default_) const;

    /**
     * Retrieves a type value for the attribute matching a given tag from
     * the set.
     *
     * This is a convinience method that turns the node associated with the
     * attribute into a type. It's a fatal error if the attribute doesn't
     * carry a type value.
     *
     * default_: A default value to return of no such attribute exists in the
     * set.
     *
     * Returns: The attributes value, or the default_.
     */
    shared_ptr<Type> getAsType(attribute::Tag tag, shared_ptr<Type> type = nullptr) const;

    /**
     * Retrieves a expression value for the attribute matching a given tag from
     * the set.
     *
     * This is a convinience method that turns the node associated with the
     * attribute into a expression. It's a fatal error if the attribute doesn't
     * carry a expression value.
     *
     * default_: A default value to return of no such attribute exists in the
     * set.
     *
     * Returns: The attributes value, or the default_.
     */
    shared_ptr<Expression> getAsExpression(attribute::Tag tag, shared_ptr<Expression> default_ = nullptr) const;

    /**
     * Adds an attribute to the set. Any existing of the same tag will be
     * overriden.
     *
     * attr: The attribute to add..
     */
    void add(Attribute attr);

    /**
     * Adds an attribute with a value to the set. Any existing of the same
     * tag will be overriden.
     *
     * attr: The tag of the attribute to add.
     *
     * value: The value of the attribute to add.
     */
    void add(attribute::Tag, shared_ptr<Node> value);

    /**
     * Adds an integer attribute to the set. Any existing of the same tag
     * will be overriden.
     *
     * This is a short-cut that creates a creates a corresponding \c Node
     * object internally.
     *
     * attr: The tag of the attribute to add.
     *
     * value: The value of the attribute to add.
     */
    void add(attribute::Tag, int64_t value);

    /**
     * Adds a string attribute to the set. Any existing of the same tag will
     * be overriden.
     *
     * This is a short-cut that creates a creates a corresponding \c Node
     * object internally.
     *
     * attr: The tag of the attribute to add.
     *
     * value: The value of the attribute to add.
     */
    void add(attribute::Tag, const std::string& value);

    /**
     * Adds an attribute without a value to the set. Any existing of the same
     * tag will be overriden.
     *
     * attr: The tag of the attribute to add.
     *
     * value: The value of the attribute to add.
     */
    void add(attribute::Tag attr);

    /**
     * Removes an attributes from the set. This removes any attribute with
     * the same tag, independent of its value.
     *
     * attr: The attribute to remove.
     */
    void remove(Attribute attr);

    /**
     * Removes an attributes from the set. This removes any attribute with
     * the given tag; it's ok for no such attribute to exist.
     *
     * attr: The attribute to remove.
     */
    void remove(attribute::Tag tag);

    /**
     * Validates an attribute set. This makes sure each attribute in the set
     * validates individually, per \a Attribute::validate().
     *
     * Returns true if correct, false otherwise.
     */
    bool validate(attribute::Context ctx, std::string* msg = nullptr) const;

    /**
     * Returns a list of all attributes contained in the set.
     */
    std::list<Attribute> all() const;

    /**
     * Returns a textual representation of the attribute corresponding to how
     * the it would be specified in HILTI source code.
     */
    std::string render() override;

    AttributeSet& operator=(const AttributeSet& other);

    ACCEPT_VISITOR_ROOT();

private:
    std::vector<Attribute> _attributes;
};

}

#endif
