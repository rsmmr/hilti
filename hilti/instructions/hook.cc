
#include "define-instruction.h"

#include "../builder/nodes.h"
#include "hook.h"

iBeginCC(hook)
    iValidateCC(DisableGroup)
    {
    }

    iDocCC(DisableGroup, R"(    
        Disables the hook group given by *op1* globally.
    )")

iEndCC

iBeginCC(hook)
    iValidateCC(EnableGroup)
    {
    }

    iDocCC(EnableGroup, R"(    
        Enables the hook group given by *op1* globally.
    )")

iEndCC

iBeginCC(hook)
    iValidateCC(GroupEnabled)
    {
    }

    iDocCC(GroupEnabled, R"(    
        Sets *target* to ``True`` if hook group *op1* is enabled, and to
        *False* otherwise.
    )")

iEndCC

iBeginCC(hook)
    iValidateCC(Run)
    {
        auto htype = ast::rtti::checkedCast<type::Hook>(op1->type());
        auto rtype = htype->result()->type();
        checkCallParameters(htype, op2);
        checkCallResult(rtype, target ? target->type() : nullptr);
    }

    iFlowInfoCC(Run)
    {
        fi.modified = util::set_union(fi.modified, fi.defined);
        return fi;
    }

    iDocCC(Run, R"(    
        Executes the hook *op1* with arguments *op2*, storing the hook's
        return value in *target*.
    )")

iEndCC

iBeginCC(hook)
    iValidateCC(Stop)
    {
        // To find out if hook.stop is used outside of a hook, we walk up the
        // current path. If we aren't the child of an expression, the we need
        // to find a hook declaration.
        auto nodes = validator()->currentNodes();

        shared_ptr<declaration::Hook> hook = nullptr;
        bool ok = false;

        for ( auto i = nodes.rbegin(); i != nodes.rend(); i++ ) {
            auto n = *i;

            hook = ast::rtti::tryCast<declaration::Hook>(n);

            if ( hook ) {
                ok = true;
                break;
            }

            if ( ast::rtti::isA<Expression>(n) ) {
                ok = true;
                break;
            }
        }

        if ( ! ok ) {
            error(nullptr, "hook.stop must only be used inside a hook");
            return;
        }

        if ( ! hook )
            return;

        auto htype = hook->hook()->type();
        auto rtype = htype->result()->type();
        checkCallResult(rtype, op1);
    }

    iSuccessorsCC(Stop)
    {
        return std::set<shared_ptr<Expression>>();
    }

    iDocCC(Stop, R"(    
        Stops the execution of the current hook and returns *op1* as the hooks
        value (if one is needed).
    )")

iEndCC
