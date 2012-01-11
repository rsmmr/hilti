
Implementation Overview
=======================

AST Infrastructure [Mostly missing]
-----------------------------------

Fragments:

HILTI uses a sub-library for building, managing, and traversing ASTs.
The AST library provides classes and other infrastructure that is
common to ASTs for imperative languages while leaving language
specifics (such as what types and expressions to offer) open to
extension. The two main mechanism to make this work are (1) the actual
AST classes to use are passed into the library as template parameters;
and (2) the library provides a set of mix-in classes that can be
combined with local classes to provide pre-build functionality.

Mix-ins all derive from expression::MixIn and their methods mimic those
here. As the user of expression instances, you'll normally not interact
with the mix-in but just with the Expression class, which will forward
calls as necessary. When you want to override any methods, likewise just
override the version in Expression.

