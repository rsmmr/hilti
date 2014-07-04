
/// Internal top-level include file. From user code, use hilti.h instead.

#ifndef HILTI_INTERN_H
#define HILTI_INTERN_H

#include <fstream>

#include "common.h"
#include "context.h"
#include "module.h"
#include "constant.h"
#include "ctor.h"
#include "declaration.h"
#include "expression.h"
#include "function.h"
#include "id.h"
#include "instruction.h"
#include "pass.h"
#include "statement.h"
#include "type.h"
#include "variable.h"
#include "attribute.h"

#include "passes/passes.h"
#include "builder/builder.h"
#include "codegen/codegen.h"
#include "codegen/linker.h"
#include "codegen/protogen.h"
#include "codegen/asm-annotater.h"

// Must come last.
#include "visitor-interface.h"

namespace llvm { class Module; }

namespace hilti {

/// Returns the current HILTI version.
extern string version();

/// Initializes the HILTI system. Must be called before any other function.
extern void init();

/// Returns a list of all HILTI instructions.
typedef InstructionRegistry::instr_list instruction_list;

instruction_list instructions();

}

#endif
