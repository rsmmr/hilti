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

iBeginH(classifier, New, "new")
    iTarget(optype::refClassifier)
    iOp1(optype::typeClassifier, true)
iEndH


iBeginH(classifier, Add, "classifier.add")
    iOp1(optype::refClassifier, false)
    iOp2(optype::any, true)
    iOp3(optype::any, true)
iEndH

iBeginH(classifier, Compile, "classifier.compile")
    iOp1(optype::refClassifier, false)
iEndH

iBeginH(classifier, Get, "classifier.get")
    iTarget(optype::any)
    iOp1(optype::refClassifier, true)
    iOp2(optype::any, true)
iEndH

iBeginH(classifier, Matches, "classifier.matches")
    iTarget(optype::boolean)
    iOp1(optype::refClassifier, true)
    iOp2(optype::any, true)
iEndH

