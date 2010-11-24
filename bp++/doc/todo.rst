
ToDo
====

Code Generator
--------------

- Once HILTI has support for tracking higher-level location
  information, the codegen should generate that.

Code Structure
--------------

- Introduce container type and derive "List" from it, moving all
  general container functionality into the base blase (including
  control hooks).

- I'm wondering whether lists (or more generally container should have a
  UnitField as their type, rather than replicating all of a field's functionalty. 

- The &default attributes isn't handled consistently, as currently each type
  needs to declare it supported itself. This attribute should generally be
  supported for all types with ctors. 

- Likewise, other attributes might be suitable for automatic support
  as well (e.g., &convert).

- Not sure the distinction between ParseableType and Type makes sense anymore.
  We should move all of ParseableType into the baseclass and use validate() to
  check whether features are used that only make sense within a unit field. 

Cleanup
-------

- A number of those cleanup tasks applied to HILTI aren't yet done
  for BinPAC. Examples:

  * The restructuring of validate into validate() and _validate
    (same for resolve, etc.)
    
  * Has the automatic recursion detection been removed?
