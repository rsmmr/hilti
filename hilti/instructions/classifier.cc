
#include "instructions/define-instruction.h"

#include "classifier.h"
#include "module.h"
#include "builder/nodes.h"

static void _validateRuleValue(const Instruction* i, shared_ptr<Type> rtype, shared_ptr<Expression> op)
{
    shared_ptr<Type> stype = nullptr;
    auto ttype = ast::as<type::Tuple>(op->type());

    if ( ! ttype ) {
        i->error(op, "operand must be tuple (struct, int)");
        return;
    }

    if ( ttype->typeList().size() != 2 ) {
        i->error(ttype, "tuple must be of type (struct, int)");
        return;
    }

    auto params = ttype->typeList();
    auto j = params.begin();

    stype = *j++;
    auto itype = *j++;

    if ( ! ast::isA<type::Integer>(itype) ) {
        i->error(ttype, "tuple must be of type (struct, int)");
        return;
    }

    assert(stype);

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

    auto rl = r->typeList();
    auto sl = s->typeList();

    for ( auto j : ::util::zip2(rl, sl) ) {
        if ( j.first->equal(j.second) )
            continue;

        bool match = false;

        auto ct = ast::as<type::trait::Classifiable>(j.first);
        assert(ct);

        for ( auto t : ct->alsoMatchableTo() ) {
            if ( i->canCoerceTo(j.second, t) )
                match = true;
        }

        if ( ! match ) {
            i->error(op, "rule value element not compatible with field type");
            return;
        }
    }
}

iBeginCC(classifier)
    iValidateCC(New) {
    }

    iDocCC(New, R"(
        Allocates a new classifier of type *op1*. Once allocated, rules can be
        added with ``classifier.add``.
    )")

iEndCC

iBeginCC(classifier)
    iValidateCC(Add) {
        auto rtype = ast::as<type::Classifier>(referencedType(op1))->ruleType();
        assert(rtype);

        _validateRuleValue(this, rtype, op2);
    }

    iDocCC(Add, R"(
        Adds a rule to classifier *op1*. The first element of *op2* is the
        rule's fields, and the second element is the rule's match priority. If
        multiple rules match with a later lookup, the rule with the highest
        priority will win. *op3* is the value that will be associated with the
        rule.  This instruction must not be called anymore after
        ``classifier.compile`` has been executed.
    )")

iEndCC

iBeginCC(classifier)
    iValidateCC(Compile) {
    }

    iDocCC(Compile, R"(
        Freezes the classifier *op1* and prepares it for subsequent lookups.
    )")

iEndCC

iBeginCC(classifier)
    iValidateCC(Get) {
        auto rtype = ast::as<type::Classifier>(referencedType(op1))->ruleType();
        canCoerceTo(op2, builder::reference::type(rtype));
    }

    iDocCC(Get, R"(
        Returns the value associated with the highest-priority rule matching
        *op2* in classifier *op1*. Throws an IndexError exception if no match
        is found. If multiple rules of the same priority match, it is
        undefined which one will be selected.  This instruction must only be
        used after the classifier has been fixed with ``classifier.compile``.
    )")

iEndCC

iBeginCC(classifier)
    iValidateCC(Matches) {
        auto rtype = ast::as<type::Classifier>(referencedType(op1))->ruleType();
        canCoerceTo(op2, builder::reference::type(rtype));
    }

    iDocCC(Matches, R"(
        Returns True if *op2* matches any rule in classifier *op1*.  This
        instruction must only be used after the classifier has been fixed with
        ``classifier.compile``.
    )")

iEndCC


