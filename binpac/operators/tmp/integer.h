
#ifndef _DA_HOME_ROBIN_WORK_BINPACPP_BP___HILTI2_BINPAC___OPERATORS
#define _DA_HOME_ROBIN_WORK_BINPACPP_BP___HILTI2_BINPAC___OPERATORS

#include "operator.h"

namespace operator_ {

#line 7 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"

namespace integer {

class Plus : public binpac::Operator
{
public:
    Plus();
    virtual ~Plus();
protected:

    shared_ptr<Type> __typeOp1() const;

#line 8 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
    shared_ptr<Type> __typeOp2() const;

#line 11 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
    string __doc() const override;

#line 13 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
    shared_ptr<Type> __typeResult(const expression_list& ops);

#line 23 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
};

}


#line 25 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"

namespace integer {

class Minus : public binpac::Operator
{
public:
    Minus();
    virtual ~Minus();
protected:

    shared_ptr<Type> __typeOp1() const;

#line 26 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
    shared_ptr<Type> __typeOp2() const;

#line 29 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
    string __doc() const override;

#line 31 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
    shared_ptr<Type> __typeResult(const expression_list& ops);

#line 41 "/da/home/robin/work/binpacpp-bp++/hilti2/binpac++/operators/integer.h"
};

}


}

#endif


