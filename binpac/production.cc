
#include <stdlib.h>
#include <cxxabi.h>

#include <util/util.h>

#include "expression.h"
#include "production.h"

using namespace binpac;
using namespace binpac::production;

Production::Production(const string& symbol, shared_ptr<Type> type, const Location& l)
{
    assert(symbol.size());
    _symbol = symbol;
    _type = type;
    _location = l;
}

const string& Production::symbol() const
{
    return _symbol;
}

shared_ptr<Type> Production::type() const
{
    return _type;
}

bool Production::nullable() const
{
    return false;
}

string Production::render() const
{
    string location = "";

    int status;
    char *name = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
    name = strrchr(name, ':');
    assert(name);

    if ( _location != Location::None )
        location = util::fmt(" (%s)", string(_location).c_str());

    return util::fmt("%10s: %-3s -> %s%s", ++name, _symbol.c_str(), renderProduction().c_str(), location.c_str());
}

const Location& Production::location() const
{
    return _location;
}

Production::ParserGenMeta* Production::pgMeta()
{
    return &_pgmeta;
}

bool Production::eodOk() const
{
    return nullable();
}

void Production::simplify()
{
}

Epsilon::Epsilon(const Location& l) : Production("<eps>", nullptr, l)
{
}

string Epsilon::renderProduction() const
{
    return "()";
}

bool Epsilon::nullable() const
{
    return true;
}

Terminal::Terminal(const string& symbol, shared_ptr<Type> type, shared_ptr<Expression> expr, filter_func filter, const Location& l)
    : Production(symbol, type, l)
{
    _expr = expr;
    _filter = filter;
}

Terminal::filter_func Terminal::filter() const
{
    return _filter;
}

shared_ptr<Expression> Terminal::expression() const
{
    return _expr;
}

void Terminal::setSink(shared_ptr<Expression> sink)
{
    _sink = sink;
}

shared_ptr<Expression> Terminal::sink() const
{
    return _sink;
}

int Literal::_id_counter = 0;

Literal::Literal(const string& symbol, shared_ptr<Type> type, shared_ptr<Expression> expr, filter_func filter, const Location& l)
    : Terminal(symbol, expr ? expr->type() : type, expr, filter, l)
{
    _id = ++_id_counter;
}

int Literal::tokenID() const
{
    return _id;
}

production::Constant::Constant(const string& symbol, shared_ptr<binpac::Constant> constant, shared_ptr<Expression> expr, filter_func filter, const Location& l)
    : Literal(symbol, ast::type::checkedTrait<type::trait::Parseable>(constant->type())->fieldType(), expr, filter, l)
{
    _const = constant;
    addChild(_const);
}

shared_ptr<binpac::Constant> production::Constant::constant() const
{
    return _const;
}

string production::Constant::renderProduction() const
{
    return _const->render() + util::fmt(" (%s)", type()->render());
}

shared_ptr<Expression> production::Constant::literal() const
{
    return std::make_shared<expression::Constant>(_const);
}

Literal::pattern_list production::Constant::patterns() const
{
    return {};
}

production::Ctor::Ctor(const string& symbol, shared_ptr<binpac::Ctor> ctor, shared_ptr<Expression> expr, filter_func filter, const Location& l)
    : Literal(symbol, ast::type::checkedTrait<type::trait::Parseable>(ctor->type())->fieldType(), expr, filter, l)
{
    _ctor = ctor;
    addChild(_ctor);
}

shared_ptr<binpac::Ctor> production::Ctor::ctor() const
{
    return _ctor;
}

string production::Ctor::renderProduction() const
{
    return _ctor->render() + util::fmt(" (%s)", type()->render());
}

shared_ptr<Expression> production::Ctor::literal() const
{
    return std::make_shared<expression::Ctor>(_ctor);
}

Literal::pattern_list production::Ctor::patterns() const
{
    return _ctor->patterns();
}

production::Variable::Variable(const string& symbol, shared_ptr<Type> type, shared_ptr<Expression> expr, filter_func filter, const Location& l)
    : Terminal(symbol, type, expr, filter, l)
{
}

string production::Variable::renderProduction() const
{
    return util::fmt("(type %s)", type()->render().c_str());
}

NonTerminal::NonTerminal(const string& symbol, shared_ptr<Type> type, const Location& l)
    : Production(symbol, type, l)
{
}

ChildGrammar::ChildGrammar(const string& symbol, shared_ptr<Production> child, shared_ptr<type::Unit> type, const Location& l)
    : NonTerminal(symbol, type, l)
{
    _child = child;
}

shared_ptr<type::Unit> ChildGrammar::childType() const
{
    return ast::checkedCast<type::Unit>(type());
}

#if 0
void ChildGrammar::setParameters(const expression_list& params)
{
    _params = params;
}

const expression_list& ChildGrammar::parameters() const
{
    return _params;
}
#endif

string ChildGrammar::renderProduction() const
{
    return _child->symbol();
}

NonTerminal::alternative_list ChildGrammar::rhss() const
{
    alternative_list rhss = { { _child } };
    return rhss;
}

Sequence::Sequence(const string& symbol, const production_list& seq, shared_ptr<Type> type, const Location& l)
    : NonTerminal(symbol, type, l)
{
    _seq = seq;
}

const Sequence::production_list& Sequence::sequence() const
{
    return _seq;
}

void Sequence::add(shared_ptr<Production> prod)
{
    _seq.push_back(prod);
}

string Sequence::renderProduction() const
{
    string s = "";
    bool first = true;

    for ( auto p : _seq ) {
        if ( ! first )
            s += " ";

        s += p->symbol();
        first = false;
    }

    return s;
}

NonTerminal::alternative_list Sequence::rhss() const
{
    alternative_list l = { _seq };
    return l;
}

LookAhead::LookAhead(const string& symbol, shared_ptr<Production> alt1, shared_ptr<Production> alt2, const Location& l)
    : NonTerminal(symbol, nullptr, l)
{
    _alts = std::make_pair(alt1, alt2);
}

std::pair<LookAhead::look_aheads, LookAhead::look_aheads> LookAhead::lookAheads() const
{
    return _lahs;
}

void LookAhead::setLookAheads(const look_aheads& lah1, const look_aheads& lah2)
{
    _lahs = std::make_pair(lah1, lah2);
}

std::pair<shared_ptr<Production>, shared_ptr<Production>> LookAhead::alternatives() const
{
    return _alts;
}

void LookAhead::setAlternatives(shared_ptr<Production> alt1, shared_ptr<Production> alt2)
{
    _alts = std::make_pair(alt1, alt2);
}

static string _fmtAlt(const production::LookAhead* p, int i)
{
    shared_ptr<Production> alt = nullptr;
    production::LookAhead::look_aheads lah;

    if ( i == 0 ) {
        alt = p->alternatives().first;
        lah = p->lookAheads().first;
    }

    else {
        alt = p->alternatives().second;
        lah = p->lookAheads().second;
    }

    string lahs;
    bool first = true;

    for ( auto l : lah ) {
        if ( ! first )
            lahs += ", ";

        auto lit = std::dynamic_pointer_cast<production::Literal>(l);
        if ( lit )
            lahs += lit->literal()->render();
        else
            lahs += l->symbol();

        first = false;
    }

    return util::fmt("{%s}: %s", lahs.c_str(), alt->symbol().c_str());
}


string LookAhead::renderProduction() const
{
    return _fmtAlt(this, 0) + " | " + _fmtAlt(this, 1);
}

NonTerminal::alternative_list LookAhead::rhss() const
{
    return { { _alts.first }, { _alts.second } };
}

Boolean::Boolean(const string& symbol, shared_ptr<Expression> expr, shared_ptr<Production> alt1, shared_ptr<Production> alt2, const Location& l)
    : NonTerminal(symbol, nullptr, l)
{
    _expr = expr;
    _branches = std::make_pair(alt1, alt2);
}

shared_ptr<Expression> Boolean::expression() const
{
    return _expr;
}

std::pair<shared_ptr<Production>, shared_ptr<Production>> Boolean::branches() const
{
    return _branches;
}

string Boolean::renderProduction() const
{
    return util::fmt("true: %s / false: %s", _branches.first->symbol().c_str(), _branches.second->symbol().c_str());
}

NonTerminal::alternative_list Boolean::rhss() const
{
    return { { _branches.first }, { _branches.second } };
}

bool Boolean::eodOk() const
{
    // Always false. If one of the branches is ok with no data, it will
    // indicate so itself.
    return false;
}

Counter::Counter(const string& symbol, shared_ptr<Expression> expr, shared_ptr<Production> body, const Location& l)
    : NonTerminal(symbol, nullptr, l)
{
    _expr = expr;
    _body = body;
}

shared_ptr<Expression> Counter::expression() const
{
    return _expr;
}

shared_ptr<Production> Counter::body() const
{
    return _body;
}

string Counter::renderProduction() const
{
    return util::fmt("counter(%s): %s", _expr->render().c_str(), _body->symbol().c_str());
}

NonTerminal::alternative_list Counter::rhss() const
{
    return { { _body } };
}

While::While(const string& symbol, shared_ptr<Expression> expr, shared_ptr<Production> body, const Location& l)
    : NonTerminal(symbol, nullptr, l)
{
    _expr = expr;
    _body = body;
}

shared_ptr<Expression> While::expression() const
{
    return _expr;
}

shared_ptr<Production> While::body() const
{
    return _body;
}

string While::renderProduction() const
{
    return util::fmt("while(%s): %s", _expr->render().c_str(), _body->symbol().c_str());
}

NonTerminal::alternative_list While::rhss() const
{
    return { { _body } };
}

Loop::Loop(const string& symbol, shared_ptr<Production> body, const Location& l)
    : NonTerminal(symbol, nullptr, l)
{
    _body = body;
}

shared_ptr<Production> Loop::body() const
{
    return _body;
}

string Loop::renderProduction() const
{
    return util::fmt("loop: %s", _body->symbol().c_str());
}

NonTerminal::alternative_list Loop::rhss() const
{
    return { { _body } };
}

Switch::Switch(const string& symbol, shared_ptr<Expression> expr, const case_list& cases, shared_ptr<Production> default_, const Location& l)
    : NonTerminal(symbol, nullptr, l)
{
    _expr = expr;
    _cases = cases;
    _default = default_;
}

shared_ptr<Expression> Switch::expression() const
{
    return _expr;
}

const Switch::case_list& Switch::alternatives() const
{
    return _cases;
}

shared_ptr<Production> Switch::default_() const
{
    return _default;
}

string Switch::renderProduction() const
{
    string r;

    if ( _default )
        r += util::fmt("* -> %s", _default->symbol().c_str());

    for ( auto c : _cases ) {

        std::list<string> exprs;

        for ( auto e : c.first )
            exprs.push_back(e->render());

        r += util::fmt(" / [%s] -> %s", util::strjoin(exprs, ","), c.second->symbol().c_str());
    }

    return r;
}

NonTerminal::alternative_list Switch::rhss() const
{
    alternative_list alts;

    for ( auto c : _cases )
        alts.push_back({ c.second });

    return alts;
}

bool Switch::eodOk() const
{
    // Always false. If one of the branches is ok with no data, it will
    // indicate so itself.
    return false;
}
