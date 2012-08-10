
// @TEST-IGNORE

shared_ptr<binpac::production::Literal> literal(const char* sym, const char* val)
{
    auto c = std::make_shared<binpac::expression::Constant>(std::make_shared<binpac::constant::String>(val));
    return std::make_shared<binpac::production::Literal>(sym, c);
}

shared_ptr<binpac::production::Sequence> sequence(const char* sym, const binpac::Production::production_list seq)
{
    return std::make_shared<binpac::production::Sequence>(sym, seq);
}

shared_ptr<binpac::production::LookAhead> lookAhead(const char* sym, shared_ptr<binpac::Production> p1, shared_ptr<binpac::Production> p2)
{
    return std::make_shared<binpac::production::LookAhead>(sym, p1, p2);
}

shared_ptr<binpac::production::Epsilon> epsilon()
{
    return std::make_shared<binpac::production::Epsilon>();
}

shared_ptr<binpac::production::Variable> variable(const char* sym, shared_ptr<binpac::Type> type)
{
    return std::make_shared<binpac::production::Variable>(sym, type);
}
