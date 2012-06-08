//
/// \type Channel.
///
/// A data type for performing packet classification, i.e., find the
/// first match out of a collection of firewall-style rules.
///
/// TODO.
///
/// Note: for bytes, we do prefix-based matching.
///
/// \ctor X
///
/// \cproto hlt_classifier*
///

#include "instructions/define-instruction.h"

static void _validateRuleValue(const Instruction* i, shared_ptr<Type> rtype, shared_ptr<Expression> op)
{
    shared_ptr<Type> stype = nullptr;
    auto ttype = ast::as<type::Tuple>(op->type());

    if ( ! ttype )
        stype = op->type();

    else {
        if ( ttype->typeList().size() != 2 ) {
            i->error(ttype, "tuple must be of type (struct, int)");
            return;
        }

        auto j = ttype->typeList().begin();

        stype = *j++;

        if ( ! ast::isA<type::Integer>(*j) ) {
            i->error(ttype, "tuple must be of type (struct, int)");
            return;
        }
    }

    auto ref = ast::as<type::Reference>(stype);

    if ( ! ref ) {
        i->error(op, "rule value must be a reference to a struct");
        return;
    }

    auto s = ast::as<type::Struct>(ref->argType());
    auto r = ast::as<type::Struct>(rtype);
    assert(r);

    if ( ! s ) {
        i->error(op, "rule value must be a reference to a struct");
        return;
    }

    if ( r->typeList().size() != s->typeList().size() ) {
        i->error(op, "rule value has wrong number of elements");
        return;
    }

    auto ri = r->typeList().begin();
    auto si = s->typeList().begin();

    for ( ; ri != r->typeList().end(); ri++, si++ ) {
        if ( (*ri)->equal(*si) )
            continue;

        bool match = false;
        for ( auto t : ast::as<type::trait::Classifiable>(*ri)->alsoMatchableTo() ) {
            if ( i->canCoerceTo(*si, t) )
                match = true;
        }

        if ( ! match ) {
            i->error(op, "rule value element not compatible with field type");
            return;
        }
    }
}

iBegin(classifier, New, "new")
    iTarget(optype::refClassifier)
    iOp1(optype::typeClassifier, true)

    iValidate {
    }

    iDoc(R"(
        Allocates a new classifier of type *op1*. Once allocated, rules can be
        added with ``classifier.add``.
    )")

iEnd


iBegin(classifier, Add, "classifier.add")
    iOp1(optype::refClassifier, false)
    iOp2(optype::any, true)
    iOp3(optype::any, true)

    iValidate {
        auto rtype = ast::as<type::Classifier>(referencedType(op1))->ruleType();
        _validateRuleValue(this, rtype, op2);
    }

    iDoc(R"(
        Adds a rule to classifier *op1*. The first element of *op2* is the
        rule's fields, and the second element is the rule's match priority. If
        multiple rules match with a later lookup, the rule with the highest
        priority will win. *op3* is the value that will be associated with the
        rule.  This instruction must not be called anymore after
        ``classifier.compile`` has been executed.
    )")

iEnd

iBegin(classifier, Compile, "classifier.compile")
    iOp1(optype::refClassifier, false)

    iValidate {
    }

    iDoc(R"(
        Freezes the classifier *op1* and prepares it for subsequent lookups.
    )")

iEnd

iBegin(classifier, Get, "classifier.get")
    iTarget(optype::any)
    iOp1(optype::refClassifier, true)
    iOp2(optype::any, true)

    iValidate {
        auto rtype = ast::as<type::Classifier>(referencedType(op1))->ruleType();
        _validateRuleValue(this, rtype, op2);
    }

    iDoc(R"(
        Returns the value associated with the highest-priority rule matching
        *op2* in classifier *op1*. Throws an IndexError exception if no match
        is found. If multiple rules of the same priority match, it is
        undefined which one will be selected.  This instruction must only be
        used after the classifier has been fixed with ``classifier.compile``.
    )")

iEnd

iBegin(classifier, Matches, "classifier.matches")
    iTarget(optype::boolean)
    iOp1(optype::refClassifier, true)
    iOp2(optype::any, true)

    iValidate {
        auto rtype = ast::as<type::Classifier>(referencedType(op1))->ruleType();
        _validateRuleValue(this, rtype, op2);
    }

    iDoc(R"(
        Returns True if *op2* matches any rule in classifier *op1*.  This
        instruction must only be used after the classifier has been fixed with
        ``classifier.compile``.
    )")

iEnd

