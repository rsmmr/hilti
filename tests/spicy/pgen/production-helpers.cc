
// @TEST-IGNORE

shared_ptr<spicy::production::Literal> literal(const char* sym, const char* val)
{
    auto c = std::make_shared<spicy::ctor::Bytes>(val);
    return std::make_shared<spicy::production::Ctor>(sym, c);
}

shared_ptr<spicy::production::Sequence> sequence(const char* sym, const spicy::Production::production_list seq)
{
    return std::make_shared<spicy::production::Sequence>(sym, seq);
}

shared_ptr<spicy::production::LookAhead> lookAhead(const char* sym, shared_ptr<spicy::Production> p1, shared_ptr<spicy::Production> p2)
{
    return std::make_shared<spicy::production::LookAhead>(sym, p1, p2);
}

shared_ptr<spicy::production::Epsilon> epsilon()
{
    return std::make_shared<spicy::production::Epsilon>();
}

shared_ptr<spicy::production::Variable> variable(const char* sym, shared_ptr<spicy::Type> type)
{
    return std::make_shared<spicy::production::Variable>(sym, type);
}
