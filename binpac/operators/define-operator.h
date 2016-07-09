
#ifndef BINPAC_OPERATORS_DEFINE_OPERATOR_H
#define BINPAC_OPERATORS_DEFINE_OPERATOR_H

#define opBeginH(OP) class OP : public binpac::Operatpr {
public:
OP();
virtual ~OP();

protected:
bool __matchOperands(const expression_list& ops);
void __validate(const expression_list& ops);
;
shared_ptr<Expression> __simplify(const expression_list& exprs);
string __doc() const
{
    return "<No documentation>";
}
shared_ptr<Type> __typeResult(const expression_list& exprs);

class Plus : public Operator {
public:
    Plus();

protected:
    shared_ptr<Type> __typeOp1() const
    {
        return;
    }
    shared_ptr<Type> __typeOp2() const
    {
        return std::make_shared<type::Integer>();
    }
};


Plus::Plus : Operator(operator_::Plus)
{
    __type_op1 = ;
    __type_op2 = std::make_shared<type::Integer>();
}

shared_ptr<Type> Plus::typeResult(const expression_list& exprs) const
{
}

opBegin(Plus)
    op1(std::make_shared<type::Integer>()) op2(std::make_shared<type::Integer>())

        typeResult(Plus)
    {
        auto op = exprs.begin();
        op1 = *op++;
        op2 = *op++;

        int w1 = ast::checkedCast<type::Integer>(op1->type())->width();
        int w2 = ast::checkedCast<type::Integer>(op2->type())->width();

        return std::make_shared<type::Integer>(max(w1, w2));
    }

    opValidate()
    {
    }
    opDoc("Adds two integers.");
opEnd


#endif
