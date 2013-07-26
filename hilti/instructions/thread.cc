
#include "define-instruction.h"

#include "thread.h"
#include "../module.h"

iBeginCC(thread)
    iValidateCC(GetContext) {
        // Checked in passes::Validator.
    }

    iDocCC(GetContext, R"(    
        Returns the thread context of the currently executing virtual thread.
        The type of the result operand must opf type ``context``, which is
        implicitly defined as current function's thread context type. The
        instruction cannot be used if no such has been defined.
    )")

iEndCC

iBeginCC(thread)
    iValidateCC(SetContext) {
        // Checked in passes::Validator.
    }

    iDocCC(SetContext, R"(    
        Sets the thread context of the currently executing virtual thread to
        *op1*. The type of *op1* must match the current module's ``context``
        definition.
    )")
iEndCC

iBeginCC(thread)
    iValidateCC(ThreadID) {
    }

    iDocCC(ThreadID, R"(    
        Returns the ID of the current virtual thread. Returns -1 if executed
        in the primary thread.
    )")

iEndCC

iBeginCC(thread)
    iValidateCC(Schedule) {
        auto ftype = as<type::Function>(op1->type());
        auto rtype = ftype->result()->type();
        shared_ptr<Type> none = nullptr;
        checkCallParameters(ftype, op2);

        if ( ! ast::isA<type::Void>(rtype) ) {
            error(op1, "function must not return a value");
        }

        // Scope promotion is checked in validator.
    }

    iDocCC(Schedule, R"(    
        Schedules a function call onto a virtual thread. If *op3* is not
        given, the current thread context determines the target thread,
        according to HILTI context-based scheduling. If *op3* is given, it
        gives the target thread ID directly; in this case the functions thread
        context will be cleared when running.
    )")

iEndCC

