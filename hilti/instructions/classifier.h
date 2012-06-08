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

namespace hilti {
namespace instruction {
namespace classifier {

// Defined in classifier.cc.
extern void _validateRuleValue(const Instruction* i, shared_ptr<Type> rtype, shared_ptr<Expression> op);

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
        assert(rtype);

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

