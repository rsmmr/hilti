
ToDo
====

Not working
-----------

- visitor support is broken. While it's not used anymore internally,
it will likely be useful for external code, so we should restore
that functionality.

Instructions
------------

- Single return statement independent of whether we're returning a
  value or not. 

- The string variants of regexp.* aren't implemented yet.

- The string type is still missing a number of instructions.

- Add support for setting location information.

Code Generation
---------------

- Generate debug information.

Infrastructure
--------------

- We don't ensure that instructions that modify their arguments
  don't receive any constants. 

- Extending the previous point, we should add hints to the
  signatures signaling which arguments are modified and how target
  and operands relate to each other (for enabling optimizations).
  
- The parser doesn't associate line-comments correctly with global
  declarations (or better: it does not do so at all; the code
  doesn't seem to do the right thing.) It works for comments before
  the "module" statement though.

- We should be able to significantly simplify the whole type
  comparision thing now (type.__equal__, type.cmpWithSameType).
  Essentually, comapring two types should yield True only if they
  are exactly equivalent (i.e., without coercion). 

- Defining HILTI-C functions in HILTI doesn't work right yet. For example,
  accessing their parameters from the HILTI body doesn't produce the right
  code.

Cleanup
-------

- Need to revisit type.__equal__. See todo there. 

- Do we still need operand.Type, now that MetaType is pretty much a standard
  type that supports all the usual operations?

- constraints.py: do we really need the per-type cSomething constraints at the
  very end? Can't we use types directly?

- For the node.Node methods, derived classes do not consistely call
  the parent's implementation.
  
- operand.py: coerceTo() shouldn't modify the operand in place but just return
  a new operand which is accordingly coerced. Otherwise, it gets confusion easily.
  
- We should cleanup which exception is thrown when; the choice isn't
  done very systemically right now. 

- Remove labels'@-prefix. We might want to introduce id.Label to
  separate them from other locals.

LLVM problems
-------------

- Can't add the tailcall attribute in llvm-py currently (fixed by
  patching llvm-py).

- At least for X86_64, our struct-return heuristic isn't sufficient.
  It does for example not return tuple<double,string> correctly.
  Worked around that particular cases by hard-coding it in system.h,
  but generally it seems we need a more complex set of rules. 
  
  Don't know what the situation is on other ABIs.

- llvm-py caches objects based on their physical address; not a good
  idea ... The cache sometimes returned objects of the wrong type.
  We patch llvm-py to avoid that (see llvm-py-objcache.diff).
  However, there's still a low probability that the wrong object is
  used ...

Low-level CodeGen Optimizations
-------------------------------

- In CodeGen.llvmGenerateCCall, when returning structs, we have to
  create a temporary on the stack just because we can't cast an
  integer into a struct. I'd think the tmp could be optimized away
  but it currently isn't. 

- The byte-order specific unpack's could use the bswap intrinsic.

- The addr's check for an IPv4 address could use the ctlz intrinsic.

- When building struct values, rather than alloc'ing tmp instances,
  which should use insertvalue.

- The map type needs to copy key and value currently, as they may be lying on
  the stack. Can we change that so that they are only copied if necessary?

See below for higher-level / more extensive optimization ideas.

Documentation
-------------

- Generating the documentation is currently broken. Need to overhaul
  that quite a bit.

- The C layer is mostly undocumneted. Need to add Doxygen comments
  across the board. 

3rd party
---------

- The Boehm-Weiser garbage collector does not seem to support 
  PARALLEL_MARK and THREAD_LOCAL_ALLOC on all platforms; notably not
  on FreeBSD. We enable always specify them in
  libhilti/scripts/do-build, but bdwgc/CMakeLists.txt may ignore
  them based on platform. Check that more carefully.

Links
-----

- Sunrise DD: A hash table implementation with lock-free concurrent
  access. Looks pretty complex though, and does not make sense to
  use until we have settled on a data model for multi-threading.
  
  http://www.sunrisetel.net/software/devtools/sunrise-data-dictionary.shtml

- Look at http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for an
  alternative (and extremely small) UTF-8 decoder. 
  
- LGPL Aho-Corasick implementation.  


Optimizations
-------------

HILTI Level
~~~~~~~~~~~

- Track which fields of a struct are actually needed. Those which are
  not read (or not used at all), can be removed for non~exported types.

  Potentially, a struct could be empty afterwards, in which case can
  remove it completely, including all references to it.

- Track which locals don't need to be saved in the function frame
  (e.g., because of potential yielding).  Remove them from the frame
  and use local LLVM (SSA~) variables during code generation
  instead.

- Track locals which have the same type but are never used
  concurrently.  They can be merged into a single local (the
  previous point may already remove a number of these, but not all).

- There are probably a number of micro~optimizations easy and
  worthwhile doing. Look at generated HILTI code.

- Dead~code elimination, in particular remove all code for hooks
  which are never run.

  While LLVM already does eliminate dead code, doing it at the HILTI
  level as well allows the other optimization above to kick in.

- Inlining at the HILTI level; again, this will allow more
  optimizations to kick in.

- Can we identify cases where we can combine nested structures into
  a single one? Might be hard to do in general, but seems there
  could a few specific cases, particularly coming out of BinPAC,
  where it will be helpful.

- BinPAC++ uses a "__cur_ field in the parse objects to allow hooks
  to change the current parsing position. Before a hook is run, that
  field is set to the current position and afterwards its value is
  written back to the current position. A hook can change it in
  between. However, most of the time there is no change and the
  compiler should optimize then that field away.

LLVM Level
~~~~~~~~~~

- Track which of our one~function~per~block functions are called
  only from their parent function, not from external via
  continuations (or from other child functions of the same functions
  that are called from continatuions). Those can be recombined with
  their parent into a single LLVM function, removing the function
  call glue.

- Can we optimize the frame management for cases where yielding is
  unlikley? Use real locals initially and copy them into the frame
  only when necessary.

- "opt ~O1/2/3" doesn't work and creates binaries that crash with
  "illegal instruction". To reproduce: build pac~driver with HTTP
  parser on vette. 
