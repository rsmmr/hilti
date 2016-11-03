
#ifndef _USERS_ROBIN_WORK_HILTI_SPICY_OPERATORS_ADDRESS
#define _USERS_ROBIN_WORK_HILTI_SPICY_OPERATORS_ADDRESS

#include <spicy/expression.h>
#include <spicy/operator.h>


#line 1 "/Users/robin/work/hilti/spicy/operators/address.cc"

namespace spicy { namespace operator_ {
namespace address {

class Equal : public spicy::Operator
{
public:
   Equal();
   virtual ~Equal();
protected:
   virtual string __namespace() const override { return "address"; }

#line 2 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __typeOp1() const;

#line 3 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __typeOp2() const;

#line 5 "/Users/robin/work/hilti/spicy/operators/address.cc"
   string __doc() const override;

#line 7 "/Users/robin/work/hilti/spicy/operators/address.cc"
   void __validate() override;

#line 11 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __typeResult() const override;

#line 15 "/Users/robin/work/hilti/spicy/operators/address.cc"
private:
    static shared_ptr<expression::ResolvedOperator> _factory(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<spicy::Module> module, const Location& l);
};

} // namespace address
} // namespace operator_

namespace expression { namespace operator_ {
namespace address {

class Equal : public ResolvedOperator
{
public:
    Equal(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<spicy::Module> module, const Location& l);
    ACCEPT_VISITOR(::spicy::expression::ResolvedOperator);
};

} // namespace address
} // namespace resolved
} // namespace expression
} // namespace spicy


#line 17 "/Users/robin/work/hilti/spicy/operators/address.cc"

namespace spicy { namespace operator_ {
namespace address {

class Family : public spicy::Operator
{
public:
   Family();
   virtual ~Family();
protected:
   virtual string __namespace() const override { return "address"; }

#line 18 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __typeOp1() const;

#line 19 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __typeOp2() const;

#line 21 "/Users/robin/work/hilti/spicy/operators/address.cc"
   string __doc() const override;

#line 22 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __docTypeResult() const override;

#line 24 "/Users/robin/work/hilti/spicy/operators/address.cc"
   void __validate() override;

#line 28 "/Users/robin/work/hilti/spicy/operators/address.cc"
   shared_ptr<Type> __typeResult() const override;

#line 32 "/Users/robin/work/hilti/spicy/operators/address.cc"
private:
    static shared_ptr<expression::ResolvedOperator> _factory(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<spicy::Module> module, const Location& l);
};

} // namespace address
} // namespace operator_

namespace expression { namespace operator_ {
namespace address {

class Family : public ResolvedOperator
{
public:
    Family(shared_ptr<Operator> op, const expression_list& ops, shared_ptr<spicy::Module> module, const Location& l);
    ACCEPT_VISITOR(::spicy::expression::ResolvedOperator);
};

} // namespace address
} // namespace resolved
} // namespace expression
} // namespace spicy



#endif


