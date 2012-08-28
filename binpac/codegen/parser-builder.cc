
#include <hilti.h>

#include "parser-builder.h"
#include "expression.h"
#include "grammar.h"
#include "production.h"
#include "statement.h"
#include "type.h"
#include "declaration.h"
#include "attribute.h"

using namespace binpac;
using namespace binpac::codegen;

// A set of helper function to create HILTI types.

shared_ptr<hilti::Type> _hiltiTypeBytes()
{
    return hilti::builder::reference::type(hilti::builder::bytes::type());
}

shared_ptr<hilti::Type> _hiltiTypeIteratorBytes()
{
    return hilti::builder::iterator::typeBytes();
}

shared_ptr<hilti::Type> _hiltiTypeLookAhead()
{
    return hilti::builder::integer::type(32);
}

shared_ptr<hilti::Type> _hiltiTypeCookie()
{
    return hilti::builder::reference::type(hilti::builder::type::byName("BinPAC::UserCookie"));
}

shared_ptr<hilti::Type> _hiltiTypeParseResult()
{
    // (__cur, __lahead, __lahstart).
    hilti::builder::type_list ttypes = { _hiltiTypeIteratorBytes(), _hiltiTypeLookAhead(), _hiltiTypeIteratorBytes() };
    return hilti::builder::tuple::type(ttypes);
}

// Returns the value representin "no look-ahead""
shared_ptr<hilti::Expression> _hiltiLookAheadNone()
{
    return hilti::builder::integer::create(0);
}

// A class collecting the current set of parser arguments.
class binpac::codegen::ParserState
{
public:
    ParserState(shared_ptr<binpac::type::Unit> unit,
           shared_ptr<hilti::Expression> self = nullptr,
           shared_ptr<hilti::Expression> data = nullptr,
           shared_ptr<hilti::Expression> cur = nullptr,
           shared_ptr<hilti::Expression> lahead = nullptr,
           shared_ptr<hilti::Expression> lahstart = nullptr,
           shared_ptr<hilti::Expression> cookie = nullptr);

    shared_ptr<hilti::Expression> hiltiArguments() const;

    shared_ptr<binpac::type::Unit> unit;
    shared_ptr<hilti::Expression> self;
    shared_ptr<hilti::Expression> data;
    shared_ptr<hilti::Expression> cur;
    shared_ptr<hilti::Expression> lahead;
    shared_ptr<hilti::Expression> lahstart;
    shared_ptr<hilti::Expression> cookie;
};

ParserState::ParserState(shared_ptr<binpac::type::Unit> arg_unit,
               shared_ptr<hilti::Expression> arg_self,
               shared_ptr<hilti::Expression> arg_data,
               shared_ptr<hilti::Expression> arg_cur,
               shared_ptr<hilti::Expression> arg_lahead,
               shared_ptr<hilti::Expression> arg_lahstart,
               shared_ptr<hilti::Expression> arg_cookie)
{
    unit = arg_unit;
    self = (arg_self ? arg_self : hilti::builder::id::create("__self"));
    data = (arg_data ? arg_data : hilti::builder::id::create("__data"));
    cur = (arg_cur ? arg_cur : hilti::builder::id::create("__cur"));
    lahead = (arg_lahead ? arg_lahead : hilti::builder::id::create("__lahead"));
    lahstart = (arg_lahstart ? arg_lahstart : hilti::builder::id::create("__lahstart"));
    cookie = (arg_cookie ? arg_cookie : hilti::builder::id::create("__cookie"));
}

shared_ptr<hilti::Expression> ParserState::hiltiArguments() const
{
    hilti::builder::tuple::element_list args = { self, data, cur, lahead, lahstart, cookie };
    return hilti::builder::tuple::create(args, unit->location());
}

ParserBuilder::ParserBuilder(CodeGen* cg)
    : CGVisitor<shared_ptr<hilti::Expression>, shared_ptr<type::unit::item::Field>>(cg, "ParserBuilder")
{
}

ParserBuilder::~ParserBuilder()
{
}

void ParserBuilder::hiltiExportParser(shared_ptr<type::Unit> unit)
{
    auto parse_host = _hiltiCreateHostFunction(unit, false);
    // auto parse_sink = _hiltiCreateHostFunction(unit, true);
    _hiltiCreateParserInitFunction(unit, parse_host, parse_host);
}

void ParserBuilder::hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook)
{
    auto unit = hook->unit();
    assert(unit);

    auto i = unit->item(id);
    assert(i && i->type());
    _hiltiDefineHook(_hookForItem(unit, i), unit, hook->body(), i->type());
}

void ParserBuilder::hiltiUnitHooks(shared_ptr<type::Unit> unit)
{
    for ( auto i : unit->items() ) {
        if ( ! i->type() )
            continue;

        for ( auto h : i->hooks() ) {
            _hiltiDefineHook(_hookForItem(unit, i->sharedPtr<type::unit::Item>()),
                             unit, h->body(), i->type());
        }
    }

    for ( auto g : unit->globalHooks() ) {
        for ( auto h : g->hooks() ) {
            auto i = unit->item(g->id());
            assert(i && i->type());
            _hiltiDefineHook(_hookForItem(unit, i), unit, h->body(), i->type());
        }
    }
}

void ParserBuilder::_hiltiCreateParserInitFunction(shared_ptr<type::Unit> unit,
                                                   shared_ptr<hilti::Expression> parse_host,
                                                   shared_ptr<hilti::Expression> parse_sink)
{
    string name = util::fmt("init_%s", unit->id()->name().c_str());
    auto void_ = hilti::builder::function::result(hilti::builder::void_::type());
    auto func = cg()->moduleBuilder()->pushFunction(name, void_, {});
    func->function()->setInitFunction();

    auto parser = _hiltiParserDefinition(unit);
    auto funcs = cg()->builder()->addLocal("__funcs", hilti::builder::tuple::type({ hilti::builder::caddr::type(), hilti::builder::caddr::type() }));

    hilti::builder::list::element_list mtypes;

    for ( auto p : unit->properties() ) {
        if ( p->id()->name() != "mimetype" )
            continue;

        mtypes.push_back(cg()->hiltiExpression(p->value()));
        }


    auto mime_types = hilti::builder::list::create(hilti::builder::string::type(), mtypes, unit->location());

    cg()->builder()->addInstruction(parser, hilti::instruction::struct_::New,
                                    hilti::builder::id::create("BinPAC::Parser"));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("name"),
                                    hilti::builder::string::create(unit->id()->name()));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("description"),
                                    hilti::builder::string::create("No description yet."));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("mime_types"),
                                    mime_types);

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("name"),
                                    hilti::builder::string::create(unit->id()->name()));


    auto f = cg()->builder()->addLocal("__f", hilti::builder::caddr::type());

    cg()->builder()->addInstruction(funcs, hilti::instruction::caddr::Function, parse_host);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("parse_func"), f);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("resume_func"), f);

    cg()->builder()->addInstruction(funcs, hilti::instruction::caddr::Function, parse_sink);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("parse_func_sink"), f);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, parser,
                                    hilti::builder::string::create("resume_func_sink"), f);


#if 0
    TODO
        # Set the new_func field if our parser does not receive further
        # parameters.
        if not self._grammar.params():
            name = self._type.nameFunctionNew()
            fid = hilti.operand.ID(hilti.id.Unknown(name, self.cg().moduleBuilder().module().scope()))
            builder.caddr_function(funcs, fid)
            builder.tuple_index(f, funcs, 0)
            builder.struct_set(parser, "new_func", f)

        else:
            # Set the new_func field to null.
            null = builder.addLocal("null", hilti.type.CAddr())
            builder.struct_set(parser, "new_func", null)
#endif

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPAC::register_parser"),
                                    hilti::builder::tuple::create( { parser } ));

    cg()->builder()->addInstruction(hilti::instruction::flow::ReturnVoid);

    cg()->moduleBuilder()->popFunction();
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiCreateHostFunction(shared_ptr<type::Unit> unit, bool sink)
{
    auto utype = cg()->hiltiType(unit);
    auto rtype = hilti::builder::function::result(utype);

    auto arg1 = hilti::builder::function::parameter("__data", _hiltiTypeBytes(), false, nullptr);
    auto arg2 = hilti::builder::function::parameter("__cookie", _hiltiTypeCookie(), false, nullptr);

    hilti::builder::function::parameter_list args = { arg1, arg2 };

    if ( sink )
        args.push_front(hilti::builder::function::parameter("__self", utype, false, nullptr));

    else {
        // TODO: Add unit parameters.
    }

    string name = util::fmt("parse_%s", unit->id()->name().c_str());

    if ( sink )
        name = "__" + name + "_sink";

    auto func = cg()->moduleBuilder()->pushFunction(name, rtype, args);
    cg()->moduleBuilder()->exportID(name);

    auto self = sink ? hilti::builder::id::create("__self") : _allocateParseObject(unit, false);
    auto data = hilti::builder::id::create("__data");
    auto cur = cg()->builder()->addLocal("__cur", _hiltiTypeIteratorBytes());
    auto lah = cg()->builder()->addLocal("__lahead", _hiltiTypeLookAhead(), _hiltiLookAheadNone());
    auto lahstart = cg()->builder()->addLocal("__lahstart", _hiltiTypeIteratorBytes());
    auto cookie = hilti::builder::id::create("__cookie");

    cg()->builder()->addInstruction(cur, hilti::instruction::operator_::Begin, data);

    auto pstate = std::make_shared<ParserState>(unit, self, data, cur, lah, lahstart, cookie);
    pushState(pstate);

    _prepareParseObject();

    auto presult = cg()->builder()->addLocal("__presult", _hiltiTypeParseResult());
    auto pfunc = cg()->hiltiParseFunction(unit);
    cg()->builder()->addInstruction(presult, hilti::instruction::flow::CallResult, pfunc, state()->hiltiArguments());

    _finalizeParseObject();

    cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, self);

    popState();

    return cg()->moduleBuilder()->popFunction();
}

shared_ptr<hilti::Expression> ParserBuilder::hiltiCreateParseFunction(shared_ptr<type::Unit> unit)
{
    auto grammar = unit->grammar();
    assert(grammar);

    auto name = util::fmt("parse_%s_internal", grammar->name().c_str());
    auto func = _newParseFunction(name, unit);

    if ( cg()->debugLevel() > 0 ) {
        cg()->builder()->addDebugMsg("binpac", unit->id()->name());
        cg()->builder()->debugPushIndent();
    }

    bool success = processOne(grammar->root());
    assert(success);

    if ( cg()->debugLevel() > 0 )
        cg()->builder()->debugPopIndent();

    hilti::builder::tuple::element_list elems = { state()->cur, state()->lahead, state()->lahstart };
    cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, hilti::builder::tuple::create(elems));

    popState();

    return cg()->moduleBuilder()->popFunction();
}

shared_ptr<ParserState> ParserBuilder::state() const
{
    assert(_states.size());
    return _states.back();
}

void ParserBuilder::pushState(shared_ptr<ParserState> state)
{
    _states.push_back(state);
}

void ParserBuilder::popState()
{
    assert(_states.size());
    _states.pop_back();
}

shared_ptr<hilti::Expression> ParserBuilder::_newParseFunction(const string& name, shared_ptr<type::Unit> u)
{
    auto arg1 = hilti::builder::function::parameter("__self", cg()->hiltiType(u), false, nullptr);
    auto arg2 = hilti::builder::function::parameter("__data", _hiltiTypeBytes(), false, nullptr);
    auto arg3 = hilti::builder::function::parameter("__cur", _hiltiTypeIteratorBytes(), false, nullptr);
    auto arg4 = hilti::builder::function::parameter("__lahead", _hiltiTypeLookAhead(), false, nullptr);
    auto arg5 = hilti::builder::function::parameter("__lahstart", _hiltiTypeIteratorBytes(), false, nullptr);
    auto arg6 = hilti::builder::function::parameter("__cookie", _hiltiTypeCookie(), false, nullptr);

    auto rtype = hilti::builder::function::result(_hiltiTypeParseResult());

    auto func = cg()->moduleBuilder()->pushFunction(name, rtype, { arg1, arg2, arg3, arg4, arg5, arg6 });

    auto pstate = std::make_shared<ParserState>(u,
                                                hilti::builder::id::create("__self"),
                                                hilti::builder::id::create("__data"),
                                                hilti::builder::id::create("__cur"),
                                                hilti::builder::id::create("__lahead"),
                                                hilti::builder::id::create("__lahstart"),
                                                hilti::builder::id::create("__cookie"));
    pushState(pstate);

    return std::make_shared<hilti::expression::Function>(func->function(), func->location());
}

shared_ptr<hilti::Type> ParserBuilder::hiltiTypeParseObject(shared_ptr<type::Unit> u)
{
    hilti::builder::struct_::field_list fields;

    // One struct field per non-constant unit field.
    for ( auto f : u->fields() ) {
        if ( ast::isA<type::unit::item::field::Constant>(f) )
            continue;

        auto type = cg()->hiltiType(f->type());
        assert(type);

        shared_ptr<hilti::Expression> def = nullptr;

        if ( f->default_() )
            def = cg()->hiltiExpression(f->default_());
        else
            def = cg()->hiltiDefault(f->type());

        auto sfield = hilti::builder::struct_::field(f->id()->name(), type, def, false, f->location());

        if ( f->anonymous() )
            sfield->metaInfo()->add(std::make_shared<ast::MetaNode>("opt:can-remove"));

        fields.push_back(sfield);
    }

    // One struct field per variable.
    for ( auto v : u->variables() ) {
        auto type = cg()->hiltiType(v->type());
        assert(type);

        shared_ptr<hilti::Expression> def = nullptr;

        if ( v->default_() )
            def = cg()->hiltiExpression(v->default_());
        else
            def = cg()->hiltiDefault(v->type());

        fields.push_back(hilti::builder::struct_::field(v->id()->name(), type, def, false, v->location()));
    }

    return hilti::builder::struct_::type(fields, u->location());
}

shared_ptr<hilti::Expression> ParserBuilder::_allocateParseObject(shared_ptr<Type> unit, bool store_in_self)
{
    auto rt = cg()->hiltiType(unit);
    auto ut = ast::checkedCast<hilti::type::Reference>(rt)->argType();

    auto pobj = store_in_self ? state()->self : cg()->builder()->addLocal("__pobj", rt);

    cg()->builder()->addInstruction(pobj, hilti::instruction::struct_::New, hilti::builder::type::create(ut));

    return pobj;
}

void ParserBuilder::_prepareParseObject()
{
}

void ParserBuilder::_finalizeParseObject()
{
}

void ParserBuilder::_startingProduction(shared_ptr<Production> p)
{
    cg()->builder()->addComment(util::fmt("Production: %s", util::strtrim(p->render().c_str())));
    cg()->builder()->addComment("");

    if ( cg()->debugLevel() ) {
        _hiltiDebugVerbose(util::fmt("parsing %s", util::strtrim(p->render().c_str())));
        _hiltiDebugShowToken("look-ahead", state()->lahead);
        _hiltiDebugShowInput("input", state()->cur);
    }
}

void ParserBuilder::_finishedProduction(shared_ptr<Production> p)
{
}

void ParserBuilder::_newValueForField(shared_ptr<Production> prod, shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> value)
{
    auto name = field->id()->name();

    if ( cg()->debugLevel() > 0 ) {
        auto fmt = util::fmt("%s = %%s", name);
        cg()->builder()->addDebugMsg("binpac", fmt, value);
    }

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self,
                                    hilti::builder::string::create(name), value);

    _hiltiRunHook(_hookForItem(state()->unit, field), value);
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiParserDefinition(shared_ptr<type::Unit> unit)
{
    string name = util::fmt("__binpac_parser_%s", unit->id()->name().c_str());

    auto parser = ast::tryCast<hilti::Expression>(cg()->moduleBuilder()->lookupNode("parser-definition", name));

    if ( parser )
        return parser;

    auto t = hilti::builder::reference::type(hilti::builder::type::byName("BinPAC::Parser"));
    parser = cg()->moduleBuilder()->addGlobal(name, t);
    cg()->moduleBuilder()->cacheNode("parser-definition", name, parser);
    return parser;
}

void ParserBuilder::_hiltiDebug(const string& msg)
{
    if ( cg()->debugLevel() )
        cg()->builder()->addDebugMsg("binpac", msg);
}

void ParserBuilder::_hiltiDebugVerbose(const string& msg)
{
    if ( cg()->debugLevel() )
        cg()->builder()->addDebugMsg("binpac-verbose", string("- ") + msg);
}

void ParserBuilder::_hiltiDebugShowToken(const string& tag, shared_ptr<hilti::Expression> token)
{
    if ( cg()->debugLevel() == 0 )
        return;

    cg()->builder()->addDebugMsg("binpac-verbose", "- %s is %s", hilti::builder::string::create(tag), token);
}

void ParserBuilder::_hiltiDebugShowInput(const string& tag, shared_ptr<hilti::Expression> cur)
{
    if ( cg()->debugLevel() == 0 )
        return;

    auto next = cg()->builder()->addTmp("__next5", _hiltiTypeBytes());
    cg()->builder()->addInstruction(next, hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPAC::next_bytes"),
                                    hilti::builder::tuple::create({ cur, hilti::builder::integer::create(5) }));

    cg()->builder()->addDebugMsg("binpac-verbose", "- %s is |%s...|", hilti::builder::string::create(tag), next);
}

std::pair<bool, string> ParserBuilder::_hookName(const string& path)
{
    bool local = false;
    auto name = path;

    // If the module part of the ID matches the current module, remove.
    auto curmod = cg()->moduleBuilder()->module()->id()->name();

    if ( util::startsWith(name, curmod + "::") ) {
        local = true;
        name = name.substr(curmod.size() + 2, string::npos);
    }

    name = util::strreplace(name, "::", "_");
    name = string("__hook_") + name;

    return std::make_pair(local, name);
}

void ParserBuilder::_hiltiDefineHook(shared_ptr<ID> id, shared_ptr<type::Unit> unit, shared_ptr<Statement> body, shared_ptr<Type> dollardollar)
{
    auto t = _hookName(id->pathAsString());
    auto local = t.first;
    auto name = t.second;

    hilti::builder::function::parameter_list p = {
        hilti::builder::function::parameter("__self", cg()->hiltiTypeParseObjectRef(unit), false, nullptr),
        hilti::builder::function::parameter("__cookie", _hiltiTypeCookie(), false, nullptr)
    };

    if ( dollardollar )
        p.push_back(hilti::builder::function::parameter("__dollardollar", cg()->hiltiType(dollardollar), false, nullptr));


    cg()->moduleBuilder()->pushHook(name,
                                    hilti::builder::function::result(hilti::builder::void_::type()),
                                    p);

    if ( cg()->debugLevel() > 0 ) {
        auto msg = util::fmt("- executing hook %s@%s", id->pathAsString(), string(body->location()));
        cg()->builder()->addDebugMsg("binpac-verbose", msg);
    }

    cg()->hiltiStatement(body);

    cg()->moduleBuilder()->popHook();
}

void ParserBuilder::_hiltiRunHook(shared_ptr<ID> id, shared_ptr<hilti::Expression> dollardollar)
{
    if ( cg()->debugLevel() > 0 ) {
        auto msg = util::fmt("- triggering hook %s", id->pathAsString());
        cg()->builder()->addDebugMsg("binpac-verbose", msg);
    }

    auto t = _hookName(id->pathAsString());
    auto local = t.first;
    auto name = t.second;

    // Declare the hook if it's in our module and we don't have done that yet.
    if ( local && ! cg()->moduleBuilder()->lookupNode("hook", name) ) {
        hilti::builder::function::parameter_list p = {
            hilti::builder::function::parameter("__self", cg()->hiltiTypeParseObjectRef(state()->unit), false, nullptr),
            hilti::builder::function::parameter("__cookie", _hiltiTypeCookie(), false, nullptr)
        };

        if ( dollardollar )
            p.push_back(hilti::builder::function::parameter("__dollardollar", dollardollar->type(), false, nullptr));


        auto hook = cg()->moduleBuilder()->declareHook(name,
                                                       hilti::builder::function::result(hilti::builder::void_::type()),
                                                       p);

        cg()->moduleBuilder()->cacheNode("hook", name, hook);
    }

    // Run the hook.
    hilti::builder::tuple::element_list args = { state()->self, state()->cookie };

    if ( dollardollar )
        args.push_back(dollardollar);

    cg()->builder()->addInstruction(hilti::instruction::hook::Run,
                                    hilti::builder::id::create(name),
                                    hilti::builder::tuple::create(args));
}

shared_ptr<binpac::ID> ParserBuilder::_hookForItem(shared_ptr<type::Unit> unit, shared_ptr<type::unit::Item> item)
{
    auto id = util::fmt("%s::%s::%s", cg()->moduleBuilder()->module()->id()->name(), unit->id()->name(), item->id()->name());
    return std::make_shared<binpac::ID>(id);
}

shared_ptr<binpac::ID> ParserBuilder::_hookForUnit(shared_ptr<type::Unit> unit, const string& name)
{
    auto id = util::fmt("%s::%s::%s", cg()->moduleBuilder()->module()->id()->name(), unit->id()->name(), name);
    return std::make_shared<binpac::ID>(id);
}

////////// Visit methods.

void ParserBuilder::visit(constant::Address* a)
{
}

void ParserBuilder::visit(constant::Bitset* b)
{
}

void ParserBuilder::visit(constant::Bool* b)
{
}

void ParserBuilder::visit(constant::Double* d)
{
}

void ParserBuilder::visit(constant::Enum* e)
{
}

void ParserBuilder::visit(constant::Integer* i)
{
}

void ParserBuilder::visit(constant::Interval* i)
{
}

void ParserBuilder::visit(constant::Network* n)
{
}

void ParserBuilder::visit(constant::Port* p)
{
}

void ParserBuilder::visit(constant::String* s)
{
}

void ParserBuilder::visit(constant::Time* t)
{
}

void ParserBuilder::visit(ctor::Bytes* b)
{
}

void ParserBuilder::visit(ctor::RegExp* r)
{
}

void ParserBuilder::visit(production::Boolean* b)
{
}

void ParserBuilder::visit(production::ChildGrammar* c)
{
}

void ParserBuilder::visit(production::Counter* c)
{
}

void ParserBuilder::visit(production::Epsilon* e)
{
}

void ParserBuilder::visit(production::Literal* l)
{
}

void ParserBuilder::visit(production::LookAhead* l)
{
}

void ParserBuilder::visit(production::NonTerminal* n)
{
}

void ParserBuilder::visit(production::Sequence* s)
{
    _startingProduction(s->sharedPtr<Production>());

    for ( auto p : s->sequence() )
        processOne(p);

    _finishedProduction(s->sharedPtr<Production>());
}

void ParserBuilder::visit(production::Switch* s)
{
}

void ParserBuilder::visit(production::Terminal* t)
{
}

void ParserBuilder::visit(production::Variable* v)
{
    auto field = v->pgMeta()->field;
    assert(field);

    bool need_val = (field->id() != nullptr);

    cg()->builder()->addComment(util::fmt("Variable: %s", field->id()->name()));

    shared_ptr<hilti::Expression> value;
    processOne(v->type(), &value, field);

    _newValueForField(v->sharedPtr<Production>(), field, value);

    cg()->builder()->addComment("");
}

void ParserBuilder::visit(production::While* w)
{
}

void ParserBuilder::visit(type::Address* a)
{
}

void ParserBuilder::visit(type::Bitset* b)
{
}

void ParserBuilder::visit(type::Bool* b)
{
}

void ParserBuilder::visit(type::Bytes* b)
{
    auto field = arg1();
    auto rtype = hilti::builder::tuple::type({ _hiltiTypeBytes(), _hiltiTypeIteratorBytes() });
    auto result = cg()->builder()->addTmp("unpacked", rtype, nullptr, true);

    auto end = cg()->builder()->addTmp("end", _hiltiTypeIteratorBytes(), nullptr, true);
    cg()->builder()->addInstruction(end, hilti::instruction::operator_::End, state()->data);

    auto iters = hilti::builder::tuple::create({ state()->cur, end });

    auto len = field->attributes()->lookup("length");

    if ( len ) {
        auto op1 = iters;
        auto op2 = hilti::builder::id::create("Hilti::Packed::BytesFixed");
        auto op3 = cg()->hiltiExpression(len->value());
        cg()->builder()->addInstruction(result, hilti::instruction::operator_::Unpack, op1, op2, op3);
    }

    else
        internalError(b, "unknown unpack format in type::Bytes");

    auto result_val = cg()->builder()->addTmp("unpacked_val", _hiltiTypeBytes(), nullptr, true);
    cg()->builder()->addInstruction(result_val, hilti::instruction::tuple::Index, result, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::tuple::Index, result, hilti::builder::integer::create(1));

    setResult(result_val);
}

void ParserBuilder::visit(type::Double* d)
{
}

void ParserBuilder::visit(type::Enum* e)
{
}

void ParserBuilder::visit(type::Integer* i)
{
}

void ParserBuilder::visit(type::Interval* i)
{
}

void ParserBuilder::visit(type::List* l)
{
}

void ParserBuilder::visit(type::Network* n)
{
}

void ParserBuilder::visit(type::Port* p)
{
}

void ParserBuilder::visit(type::Set* s)
{
}

void ParserBuilder::visit(type::String* s)
{
}

void ParserBuilder::visit(type::Time* t)
{
}

void ParserBuilder::visit(type::Unit* u)
{
}

void ParserBuilder::visit(type::unit::Item* i)
{
}

void ParserBuilder::visit(type::unit::item::GlobalHook* h)
{
}

void ParserBuilder::visit(type::unit::item::Variable* v)
{
}

void ParserBuilder::visit(type::unit::item::Property* v)
{
}

void ParserBuilder::visit(type::unit::item::Field* f)
{
}

void ParserBuilder::visit(type::unit::item::field::Constant* c)
{
}

void ParserBuilder::visit(type::unit::item::field::RegExp* r)
{
}

void ParserBuilder::visit(type::unit::item::field::Switch* s)
{
}

void ParserBuilder::visit(type::unit::item::field::Type* t)
{
}

void ParserBuilder::visit(type::unit::item::field::switch_::Case* c)
{
}

void ParserBuilder::visit(type::Vector* v)
{
}
