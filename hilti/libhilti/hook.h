// $Id$

/// \addtogroup hook
/// @{

#ifndef HLT_HOOK_H
#define HLT_HOOK_H

#include <stdint.h>
#include "exceptions.h"

typedef struct __hlt_hook_state hlt_hook_state;

/// Enables or disables a hook group.
///
/// group: The group which's state to set.
///
/// enabled: 0 to disable that group, 1 to enable.
extern void hlt_hook_group_enable(int64_t group, int8_t enabled, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the current state of a hook group. If no hook function has that group, the function return "disabled".
///
/// group: The groups which's state to return.
///
/// Returns: 0 if the group is disabled, 1 if enable.
extern int8_t hlt_hook_group_is_enabled(int64_t group, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal functions to (de-)/initialize the hook system.
void __hlt_hooks_start();
void __hlt_hooks_stop();

/// @}

#endif
