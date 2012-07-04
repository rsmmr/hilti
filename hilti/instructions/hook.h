///
/// \type Hook
///
/// A ``hook`` is collection of individual hook functions, all having the
/// same signature and grouped together under a shared name. A caller can
/// execute all functions making up the hook by refering to it by that name.
/// Example:
///    
///       hook void my_hook() {
///           call Hilti::print("1st hook function");
///       }
///    
///       hook void my_hook() {
///           call Hilti::print("2nd hook function");
///       }
///    
///       ...
///    
///       hook.run my_hook ()
///    
///       ...
///    
/// This will print:
///    
///      1st hook function
///      2nd hook function.
///    
/// Hooks functions have a priority associated with them that determines the
/// order in which they are executed; those with higher priorities will be
/// executed first:
///    
///       hook void my_hook() &priority=-5 {
///           call Hilti::print("1st hook function");
///       }
///    
///       hook void my_hook() &priority=5 {
///           call Hilti::print("2nd hook function");
///       }
///    
///       ...
///    
///       hook.run my_hook ()
///    
///       ...
///    
/// This will print:
///    
///      2nd hook function.
///      1st hook function
///    
/// If not explicitly defined, the default priority is zero. If multiple hook
/// functions have the same priority, it is undefined in which order they are
/// executed.
///    
/// Like normal functions, hooks can receive parameters:
///    
///       hook void my_hook2(int<64> i) {
///           call Hilti::print(i)
///       }
///    
///       ...
///
///       hook.run my_hook2 (42)
///    
///       ...
///    
/// Also like normal functions, hooks can return values. However, unlike
/// normal functions, they must use the ``hook.stop`` instruction for doing
/// so, indicating that this will abort the hook's execution and not run any
/// further hook functions:
///    
///       hook int<64> my_hook3() &priority=2 {
///           call Hilti::print("1st hook function")
///           hook.stop 42
///       }
///    
///       hook void my_hook3() &priority=1 {
///           call Hilti::print("2nd hook function")
///           hook.stop 21
///       }
///    
///       ...
///    
///       local int<64> rc
///    
///       rc = hook.run my_hook3 ()
///       call Hilti::print(rc);
///    
///       ...
///    
/// This will print:
///    
///       1st hook function
///       42
///    
/// If a hook is declared to return a value, but no hook function executes
/// ``hook.stop``, the target operand will be left unchanged.
///    
/// There is also a numerical, non-negative ``group` associated with each
/// hook function:
///    
///       hook void my_hook4() &group=1 {
///           call Hilti::print("1st hook function");
///       }
///    
/// If not given, the default group is zero. All hook functions having the
/// same group can be globally disabled (and potentially later re-enabled):
///    
///       hook.disable_group 1
///    
///       ...
///    
///       hook.enable_group 1
///    
/// When a hook is run, all currently disabled hook functions are ignored.
/// Note that groups are indeed fully global, and enabling/disabling is
/// visible across all threads.

#include "instructions/define-instruction.h"

iBegin(hook, DisableGroup, "hook.disable_group")
    iOp1(optype::int64, true)

    iValidate {
    }

    iDoc(R"(    
        Disables the hook group given by *op1* globally.
    )")

iEnd

iBegin(hook, EnableGroup, "hook.enable_group")
    iOp1(optype::int64, true)

    iValidate {
    }

    iDoc(R"(    
        Enables the hook group given by *op1* globally.
    )")

iEnd

iBegin(hook, GroupEnabled, "hook.group_enabled")
    iTarget(optype::boolean)
    iOp1(optype::int64, true)

    iValidate {
    }

    iDoc(R"(    
        Sets *target* to ``True`` if hook group *op1* is enabled, and to
        *False* otherwise.
    )")

iEnd

iBegin(hook, Run, "hook.run")
    iTarget(optype::optional(optype::any))
    iOp1(optype::hook, true)
    iOp2(optype::tuple, true)

    iValidate {
        auto htype = as<type::Hook>(op1->type());
        auto rtype = htype->result()->type();
        checkCallParameters(htype, op2);
        checkCallResult(rtype, target ? target->type() : nullptr);
    }

    iDoc(R"(    
        Executes the hook *op1* with arguments *op2*, storing the hook's
        return value in *target*.
    )")

iEnd

iBegin(hook, Stop, "hook.stop")
    iTerminator()
    iOp1(optype::optional(optype::any), true)

    iValidate {
        auto hook = validator()->current<declaration::Hook>();

        if ( ! hook ) {
            error(nullptr, "hook.stop must only be used inside a hook");
            return;
        }

        auto htype = hook->hook()->type();
        auto rtype = htype->result()->type();
        checkCallResult(rtype, op1);
    }

    iDoc(R"(    
        Stops the execution of the current hook and returns *op1* as the hooks
        value (if one is needed).
    )")

iEnd

