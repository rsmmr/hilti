
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

// TODO:
//    - Replave the _last_parsed_value hack with something nicer ...

// A set of helper functions to create HILTI types.

static shared_ptr<hilti::Type> _hiltiTypeBytes()
{
    return hilti::builder::reference::type(hilti::builder::bytes::type());
}

static shared_ptr<hilti::Type> _hiltiTypeIteratorBytes()
{
    return hilti::builder::iterator::typeBytes();
}

static shared_ptr<hilti::Type> _hiltiTypeLookAhead()
{
    return hilti::builder::integer::type(32);
}

static shared_ptr<hilti::Type> _hiltiTypeParseResult(shared_ptr<hilti::Type> value_type = nullptr)
{
    // (__cur, __lahead, __lahstart [, value]).
    hilti::builder::type_list ttypes = { _hiltiTypeIteratorBytes(), _hiltiTypeLookAhead(), _hiltiTypeIteratorBytes() };

    if ( value_type )
        ttypes.push_back(value_type);

    return hilti::builder::tuple::type(ttypes);
}

static shared_ptr<hilti::Type> _hiltiTypeSink()
{
    return hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Sink"));
}

static shared_ptr<hilti::Type> _hiltiTypeParser()
{
    return hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Parser"));
}

static shared_ptr<hilti::Type> _hiltiTypeFilter()
{
    return hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::ParseFilter"));
}

// Returns the value representin "no look-ahead""
static shared_ptr<hilti::Expression> _hiltiLookAheadNone()
{
    return hilti::builder::integer::create(0);
}

static shared_ptr<hilti::Type> _hiltiTypeMatchTokenState()
{
    return hilti::builder::reference::type(hilti::builder::match_token_state::type());
}

static shared_ptr<hilti::Type> _hiltiTypeMatchResult()
{
    auto i32 = hilti::builder::integer::type(32);
    auto iter = hilti::builder::iterator::type(hilti::builder::bytes::type());
    return hilti::builder::tuple::type({i32, iter});
}

// A class collecting the current set of parser arguments.
class binpac::codegen::ParserState
{
public:
    // The mode onveys to the parsers of literals what the caller wants them
    // to do. This is needed to for doing look-ahead parsing, and hence not
    // relevant for fields that aren't literals.
    enum LiteralMode {
        // Normal parsing: parse field and raise parse error if not possible.
        DEFAULT,

        // Try to parse the field, but do not raise an error if it fails. If
        // it works, move cur as normal; if it fails, set cur to end.
        TRY,

        // We have parsed the field before as part of a look-ahead, and know
        // that it matches. Now we need to get actualy value for the iterator
        // range (lahstart, cur); the visit methods must then return the
        // parsed value.
        LAHEAD_REPARSE,
    };

    typedef std::list<shared_ptr<hilti::Expression>> parameter_list;

    ParserState(shared_ptr<binpac::type::Unit> unit,
           shared_ptr<hilti::Expression> self = nullptr,
           shared_ptr<hilti::Expression> data = nullptr,
           shared_ptr<hilti::Expression> cur = nullptr,
           shared_ptr<hilti::Expression> lahead = nullptr,
           shared_ptr<hilti::Expression> lahstart = nullptr,
           shared_ptr<hilti::Expression> cookie = nullptr,
           LiteralMode mode = DEFAULT
           );

    shared_ptr<hilti::Expression> hiltiArguments() const;

    shared_ptr<ParserState> clone() const {
        return std::make_shared<ParserState>(unit,
                                             self,
                                             data,
                                             cur,
                                             lahead,
                                             lahstart,
                                             cookie,
                                             mode);
    }

    shared_ptr<binpac::type::Unit> unit;
    shared_ptr<hilti::Expression> self;
    shared_ptr<hilti::Expression> data;
    shared_ptr<hilti::Expression> cur;
    shared_ptr<hilti::Expression> lahead;
    shared_ptr<hilti::Expression> lahstart;
    shared_ptr<hilti::Expression> cookie;
    LiteralMode mode;
};

ParserState::ParserState(shared_ptr<binpac::type::Unit> arg_unit,
               shared_ptr<hilti::Expression> arg_self,
               shared_ptr<hilti::Expression> arg_data,
               shared_ptr<hilti::Expression> arg_cur,
               shared_ptr<hilti::Expression> arg_lahead,
               shared_ptr<hilti::Expression> arg_lahstart,
               shared_ptr<hilti::Expression> arg_cookie,
               LiteralMode arg_mode
               )
{
    unit = arg_unit;
    self = (arg_self ? arg_self : hilti::builder::id::create("__self"));
    data = (arg_data ? arg_data : hilti::builder::id::create("__data"));
    cur = (arg_cur ? arg_cur : hilti::builder::id::create("__cur"));
    lahead = (arg_lahead ? arg_lahead : hilti::builder::id::create("__lahead"));
    lahstart = (arg_lahstart ? arg_lahstart : hilti::builder::id::create("__lahstart"));
    cookie = (arg_cookie ? arg_cookie : hilti::builder::id::create("__cookie"));
    mode = arg_mode;
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

bool ParserBuilder::parse(shared_ptr<Node> node, shared_ptr<hilti::Expression>* result, shared_ptr<type::unit::item::Field> f)
{
    auto prod = ast::tryCast<Production>(node);

    if ( ! prod || prod->atomic() )
        return _hiltiParse(node, result, f);

    // We wrap productions into their own functions to handle cycles
    // correctly.

    auto name = util::fmt("__parse_%s_%s", state()->unit->id()->name(), prod->symbol());
    name = util::strreplace(name, ":", "_") + (result ? "_with_result" : "");
    name = util::strreplace(name, ".", "_");

    auto func = cg()->moduleBuilder()->lookupNode("parse-func", name);

    if ( ! func ) {
        // Don't have the function yet, create it.
        auto vtype = std::make_shared<hilti::type::Unknown>();
        auto nfunc = _newParseFunction(name, state()->unit, result ? vtype : nullptr);
        cg()->moduleBuilder()->cacheNode("parse-func", name, nfunc);

        if ( ! _hiltiParse(node, result, f) )
            return false;

        if ( result )
            vtype->replace((*result)->type());

        hilti::builder::tuple::element_list elems = { state()->cur, state()->lahead, state()->lahstart };

        if ( result )
            elems.push_back(*result);

        cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, hilti::builder::tuple::create(elems));

        popState();

        cg()->moduleBuilder()->popFunction();

        func = nfunc;
    }

    // Call the function.
    auto efunc = ast::checkedCast<hilti::expression::Function>(func);

    shared_ptr<hilti::Type> vtype = nullptr;

    if ( result ) {
        auto rtype = efunc->function()->type()->result()->type();
        auto ttype = ast::checkedCast<hilti::type::Tuple>(rtype);
        auto tlist = ttype->typeList();
        auto t = tlist.begin();
        std::advance(t, 3);
        vtype = *t;
    }

    auto presult = cg()->builder()->addTmp("presult", _hiltiTypeParseResult(vtype));
    cg()->builder()->addInstruction(presult, hilti::instruction::flow::CallResult, efunc, state()->hiltiArguments());
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::tuple::Index, presult, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(state()->lahead, hilti::instruction::tuple::Index, presult, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(state()->lahstart, hilti::instruction::tuple::Index, presult, hilti::builder::integer::create(2));

    if ( result ) {
        assert(vtype);
        auto pvalue = cg()->builder()->addTmp("pvalue", vtype);
        cg()->builder()->addInstruction(pvalue, hilti::instruction::tuple::Index, presult, hilti::builder::integer::create(3));
        *result = pvalue;
    }

    return true;
}

bool ParserBuilder::_hiltiParse(shared_ptr<Node> node, shared_ptr<hilti::Expression>* result, shared_ptr<type::unit::item::Field> f)
{
    bool success = false;
    shared_ptr<type::unit::item::Field> field;
    shared_ptr<hilti::builder::BlockBuilder> cont;
    shared_ptr<hilti::builder::BlockBuilder> true_;
    shared_ptr<ParserState> pstate;

    auto prod = ast::tryCast<Production>(node);

    if ( prod )
        field = ast::tryCast<type::unit::item::Field>(prod->pgMeta()->field);

    if ( field && field->condition() ) {
        // Evaluate if() condition.
        auto blocks = cg()->builder()->addIf(cg()->hiltiExpression(field->condition()));
        true_ = std::get<0>(blocks);
        cont = std::get<1>(blocks);
        cg()->moduleBuilder()->pushBuilder(true_);
    }

    if ( field && field->attributes()->has("parse") ) {
        // Change input to what &parse attribute gives.
        auto input = cg()->hiltiExpression(field->attributes()->lookup("parse")->value());

        auto cur = cg()->builder()->addTmp("parse_cur", _hiltiTypeIteratorBytes());
        auto lah = cg()->builder()->addTmp("parse_lahead", _hiltiTypeLookAhead(), _hiltiLookAheadNone());
        auto lahstart = cg()->builder()->addTmp("parse_lahstart", _hiltiTypeIteratorBytes());

        // Argument can be either a ref<bytes> or iter<bytes>.

        shared_ptr<hilti::Expression> data = 0;

        if ( ast::tryCast<hilti::type::Iterator>(input->type()) ) {
            cg()->builder()->addInstruction(cur, hilti::instruction::operator_::Assign, input);
            data = state()->data; // TODO: This only works if the iterator is part of the same bytes object ...
        }
        else {
            cg()->builder()->addInstruction(cur, hilti::instruction::iterBytes::Begin, input);
            data = input;
        }

        pstate = std::make_shared<ParserState>(state()->unit, state()->self, data, cur, lah, lahstart, state()->cookie);
        pushState(pstate);
    }

    if ( result )
        success = processOne(node, result, f);
    else
        success = processOne(node);

    if ( pstate )
        popState();

    if ( true_ ) {
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
        cg()->moduleBuilder()->popBuilder(true_);
        cg()->moduleBuilder()->pushBuilder(cont);
    }

    return success;
}

shared_ptr<binpac::type::Unit> ParserBuilder::unit() const
{
    return state()->unit;
}

void ParserBuilder::hiltiExportParser(shared_ptr<type::Unit> unit)
{
    auto parse_host = _hiltiCreateHostFunction(unit, false);
    auto parse_sink = _hiltiCreateHostFunction(unit, true);
    _hiltiCreateParserInitFunction(unit, parse_host, parse_sink);
}

void ParserBuilder::hiltiDefineHook(shared_ptr<ID> id, shared_ptr<Hook> hook)
{
    auto unit = hook->unit();
    assert(unit);

    if ( util::startsWith(id->local(), "%") )
        _hiltiDefineHook(_hookForUnit(unit, id->local()), nullptr, false, hook->debug(), unit, hook->body(), nullptr, hook->priority());

    else {
        auto i = unit->item(id);
        assert(i && i->type());
        auto dd = hook->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;
        _hiltiDefineHook(_hookForItem(unit, i, hook->foreach(), false), i, hook->foreach(), hook->debug(), unit, hook->body(), dd, hook->priority());
    }
}

void ParserBuilder::hiltiRunFieldHooks(shared_ptr<type::unit::Item> item)
{
    _hiltiRunHook(state()->unit, _hookForItem(state()->unit, item, false, true), item, false, nullptr);
    _hiltiRunHook(state()->unit, _hookForItem(state()->unit, item, false, false), item, false, nullptr);
}

void ParserBuilder::hiltiUnitHooks(shared_ptr<type::Unit> unit)
{
    for ( auto i : unit->flattenedItems() ) {
        if ( ! i->type() )
            continue;

        for ( auto h : i->hooks() ) {
            auto dd = h->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;
            _hiltiDefineHook(_hookForItem(unit, i->sharedPtr<type::unit::Item>(), h->foreach(), true),
                             i, h->foreach(), h->debug(), unit, h->body(), dd, h->priority());
        }
    }

    for ( auto g : unit->globalHooks() ) {
        for ( auto h : g->hooks() ) {
            if ( util::startsWith(g->id()->local(), "%") )
                _hiltiDefineHook(_hookForUnit(unit, g->id()->local()), nullptr, false, h->debug(), unit, h->body(), nullptr, h->priority());

            else {
                auto i = unit->item(g->id());
                assert(i && i->type());
                auto dd = h->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;
                _hiltiDefineHook(_hookForItem(unit, i, h->foreach(), false), i, h->foreach(), h->debug(), unit, h->body(), dd, h->priority());
            }
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
    auto funcs = cg()->builder()->addTmp("funcs", hilti::builder::tuple::type({ hilti::builder::caddr::type(), hilti::builder::caddr::type() }));

    auto module = unit->firstParent<Module>();
    assert(module);

    auto fname = ::util::fmt("%s::%s", module->id()->name(), unit->id()->pathAsString(module->id()));

    hilti::builder::list::element_list mtypes;
    hilti::builder::list::element_list ports;

    auto props_mtype = unit->properties("mimetype");
    auto props_port = unit->properties("port");

    for ( auto p : props_mtype )
        mtypes.push_back(cg()->hiltiExpression(p->property()->value()));

    for ( auto p : props_port )
        ports.push_back(cg()->hiltiExpression(p->property()->value()));

    auto hilti_mtypes = hilti::builder::list::create(hilti::builder::string::type(), mtypes, unit->location());
    auto hilti_ports  = hilti::builder::list::create(hilti::builder::port::type(), ports, unit->location());

    auto descr = unit->property("description");

    shared_ptr<hilti::Expression> hilti_descr = nullptr;

    if ( descr )
        hilti_descr = cg()->hiltiExpression(descr->property()->value());
    else
        hilti_descr = hilti::builder::string::create("No description.");

    cg()->builder()->addInstruction(parser, hilti::instruction::struct_::New,
                                    hilti::builder::id::create("BinPACHilti::Parser"));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("name"),
                                    hilti::builder::string::create(fname));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("description"),
                                    hilti_descr);

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("ports"),
                                    hilti_ports);

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("params"),
                                    hilti::builder::integer::create(unit->parameters().size()));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("mime_types"),
                                    hilti_mtypes);

    auto f = cg()->builder()->addTmp("f", hilti::builder::caddr::type());

    cg()->builder()->addInstruction(funcs, hilti::instruction::caddr::Function, parse_host);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("parse_func"), f);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("resume_func"), f);

    cg()->builder()->addInstruction(funcs, hilti::instruction::caddr::Function, parse_sink);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("parse_func_sink"), f);

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("resume_func_sink"), f);


    cg()->builder()->addInstruction(funcs, hilti::instruction::caddr::Function, hiltiFunctionNew(unit));

    cg()->builder()->addInstruction(f, hilti::instruction::tuple::Index, funcs, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("new_func"),
                                    f);

    // "type_info" is initialized by "BinPACHilti::register_parser".

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
            null = builder.addTmp("null", hilti.type.CAddr())
            builder.struct_set(parser, "new_func", null)
#endif

    auto ti = hilti::builder::type::create(cg()->hiltiTypeParseObject(unit));

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::register_parser"),
                                    hilti::builder::tuple::create( { parser, ti } ));

    cg()->builder()->addInstruction(hilti::instruction::flow::ReturnVoid);

    cg()->moduleBuilder()->popFunction();
}

shared_ptr<hilti::Expression> ParserBuilder::hiltiFunctionNew(shared_ptr<type::Unit> unit)
{
    string name = util::fmt("__binpac_new_%s", unit->id()->name());

    auto func_expr = ast::tryCast<hilti::Expression>(cg()->moduleBuilder()->lookupNode("parser-new-func", name));

    if ( func_expr )
        return func_expr;

    auto utype = cg()->hiltiType(unit);
    auto rtype = hilti::builder::function::result(utype);

    hilti::builder::function::parameter_list params;

    // Add unit parameters.
    _hiltiUnitParameters(unit, &params);

    params.push_back(hilti::builder::function::parameter("__sink", _hiltiTypeSink(), false, nullptr));
    params.push_back(hilti::builder::function::parameter("__mimetype", _hiltiTypeBytes(), true, nullptr));
    params.push_back(hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), true, nullptr));

    auto func = cg()->moduleBuilder()->pushFunction(name, rtype, params);
    cg()->moduleBuilder()->exportID(name);

    auto pobj = ParserBuilder::_allocateParseObject(unit, false);
    auto pstate = std::make_shared<ParserState>(unit, pobj, nullptr, nullptr, nullptr, nullptr, hilti::builder::id::create("__cookie"));
    pushState(pstate);

    hilti_expression_type_list uparams;

    for ( auto p : state()->unit->parameters() )
        uparams.push_back(std::make_pair(hilti::builder::id::create(p->id()->name()), p->type()));

    _prepareParseObject(uparams, nullptr, hilti::builder::id::create("__sink"), hilti::builder::id::create("__mimetype"));

    cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, pobj);

    popState();
    func_expr = cg()->moduleBuilder()->popFunction();
    cg()->moduleBuilder()->cacheNode("parser-new-func", name, func_expr);

    return func_expr;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiCreateHostFunction(shared_ptr<type::Unit> unit, bool sink)
{
    auto utype = cg()->hiltiType(unit);
    auto rtype = sink ? hilti::builder::function::result(hilti::builder::void_::type())
                      : hilti::builder::function::result(utype);

    auto arg1 = hilti::builder::function::parameter("__data", _hiltiTypeBytes(), false, nullptr);
    auto arg2 = hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), false, nullptr);

    hilti::builder::function::parameter_list args = { arg1, arg2 };

    if ( sink )
        args.push_front(hilti::builder::function::parameter("__self", utype, false, nullptr));

    else
        // Add unit parameters.
        _hiltiUnitParameters(unit, &args);

    string name = util::fmt("parse_%s", unit->id()->name());

    if ( sink )
        name = "__" + name + "_sink";

    auto func = cg()->moduleBuilder()->pushFunction(name, rtype, args);
    cg()->moduleBuilder()->exportID(name);

    auto self = sink ? hilti::builder::id::create("__self") : _allocateParseObject(unit, false);
    auto data = hilti::builder::id::create("__data");
    auto cur = cg()->builder()->addTmp("cur", _hiltiTypeIteratorBytes());
    auto lah = cg()->builder()->addTmp("lahead", _hiltiTypeLookAhead(), _hiltiLookAheadNone());
    auto lahstart = cg()->builder()->addTmp("lahstart", _hiltiTypeIteratorBytes());
    auto cookie = hilti::builder::id::create("__cookie");

    cg()->builder()->addInstruction(cur, hilti::instruction::iterBytes::Begin, data);

    auto pstate = std::make_shared<ParserState>(unit, self, data, cur, lah, lahstart, cookie);
    pushState(pstate);

#if 0
    if ( sink ) {
        for ( auto p : state()->unit->parameters() ) {
            auto param = cg()->builder()->addLocal(cg()->hiltiID(p->id()), cg()->hiltiType(p->type()));
            cg()->builder()->addInstruction(param, hilti::instruction::struct_::Get, hilti::builder::string::create(util::fmt("__p_%s", p->id()->name())));
        }
    }
#endif


    if ( ! sink ) {
        hilti_expression_type_list params;

        for ( auto p : state()->unit->parameters() )
            params.push_back(std::make_pair(hilti::builder::id::create(p->id()->name()), p->type()));

        _prepareParseObject(params, state()->cur);
    }

    auto presult = cg()->builder()->addTmp("presult", _hiltiTypeParseResult());
    auto pfunc = cg()->hiltiParseFunction(unit);

    if ( cg()->debugLevel() > 0 ) {
        _hiltiDebug(unit->id()->name());
        cg()->builder()->debugPushIndent();
    }

    cg()->builder()->addInstruction(presult, hilti::instruction::flow::CallResult, pfunc, state()->hiltiArguments());

    if ( cg()->debugLevel() > 0 )
        cg()->builder()->debugPopIndent();

    if ( ! sink )
        cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, self);

    popState();

    return cg()->moduleBuilder()->popFunction();
}

void ParserBuilder::_hiltiUnitParameters(shared_ptr<type::Unit> unit, hilti::builder::function::parameter_list* args) const
{
    for ( auto p : unit->parameters() ) {
        auto type = cg()->hiltiType(p->type());

        shared_ptr<hilti::Expression> def = nullptr;

        if ( p->default_() )
            def = cg()->hiltiExpression(p->default_());

        args->push_back(hilti::builder::function::parameter(cg()->hiltiID(p->id()), type, true, def));
    }
}

shared_ptr<hilti::Expression> ParserBuilder::hiltiCreateParseFunction(shared_ptr<type::Unit> unit)
{
    auto grammar = unit->grammar();
    assert(grammar);

    auto name = util::fmt("parse_%s_internal", grammar->name().c_str());

    auto n = cg()->moduleBuilder()->lookupNode("create-parse-function", name);

    if ( n )
        return ast::checkedCast<hilti::Expression>(n);

    auto func = _newParseFunction(name, unit);
    cg()->moduleBuilder()->cacheNode("create-parse-function", name, func);

    if ( unit->exported() )
        _hiltiFilterInput(false);

    _last_parsed_value = nullptr;
    _store_values = 1;
    bool success = _hiltiParse(grammar->root(), nullptr, nullptr);
    assert(success);

    _finalizeParseObject();

    hilti::builder::tuple::element_list elems = { state()->cur, state()->lahead, state()->lahstart };
    cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, hilti::builder::tuple::create(elems));

    popState();

    cg()->moduleBuilder()->popFunction();

    return func;
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

shared_ptr<hilti::Expression> ParserBuilder::_newParseFunction(const string& name, shared_ptr<type::Unit> u, shared_ptr<hilti::Type> value_type)
{
    auto arg1 = hilti::builder::function::parameter("__self", cg()->hiltiType(u), false, nullptr);
    auto arg2 = hilti::builder::function::parameter("__data", _hiltiTypeBytes(), false, nullptr);
    auto arg3 = hilti::builder::function::parameter("__cur", _hiltiTypeIteratorBytes(), false, nullptr);
    auto arg4 = hilti::builder::function::parameter("__lahead", _hiltiTypeLookAhead(), false, nullptr);
    auto arg5 = hilti::builder::function::parameter("__lahstart", _hiltiTypeIteratorBytes(), false, nullptr);
    auto arg6 = hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), false, nullptr);

    auto rtype = hilti::builder::function::result(_hiltiTypeParseResult(value_type));

    auto func = cg()->moduleBuilder()->pushFunction(name, rtype, { arg1, arg2, arg3, arg4, arg5, arg6 });

    pushState(std::make_shared<ParserState>(u));

    return std::make_shared<hilti::expression::Function>(func->function(), func->location());
}

shared_ptr<hilti::Type> ParserBuilder::hiltiTypeParseObject(shared_ptr<type::Unit> u)
{
    hilti::builder::struct_::field_list fields;

    std::set<string> have;

    // One struct field per non-constant unit field.
    for ( auto f : u->flattenedFields() ) {
        if ( ! f->type() )
            continue;

        auto name = f->id()->name();

        if ( have.find(name) != have.end() )
            // Duplicate field names are ok with switch cases as long as the
            // types match. That (and otherwise forbidden duplicates) should
            // have already been checked where we get here ...
            continue;

        auto ftype = f->fieldType();
        auto type = cg()->hiltiType(ftype);
        assert(type);

        auto sfield = hilti::builder::struct_::field(name, type, nullptr, false, f->location());

        if ( f->anonymous() )
            sfield->setCanRemove();

        have.insert(name);
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
            def = cg()->hiltiDefault(v->type(), true, true);

        fields.push_back(hilti::builder::struct_::field(v->id()->name(), type, def, false, v->location()));
    }

    // One struct field per parameter.
    for ( auto p : u->parameters() ) {
        auto name = util::fmt("__p_%s", p->id()->name());
        auto type = cg()->hiltiType(p->type());
        auto sfield = hilti::builder::struct_::field(name, type, nullptr, false, p->location());

        fields.push_back(sfield);
    }

    // Additional fields when input buffering is enabled.
    if ( u->buffering() ) {
        // The input() position.
        auto input = hilti::builder::struct_::field("__input", _hiltiTypeIteratorBytes(), nullptr, true);

        // For passing the set_position() position back.
        auto cur   = hilti::builder::struct_::field("__cur", _hiltiTypeIteratorBytes(), nullptr, true);

        fields.push_back(input);
        fields.push_back(cur);
    }

    // Additional fields when exported to support sinks and filters.
    if ( u->exported() ) {

        // The parser object.
        auto self = hilti::builder::struct_::field("__parser", _hiltiTypeParser(), nullptr, true);

        // The sink this parser is connected to, if any.
        auto sink = hilti::builder::struct_::field("__sink", _hiltiTypeSink(), nullptr, true);

        // The MIME types associated with our input.
        auto mtype = hilti::builder::struct_::field("__mimetype", _hiltiTypeBytes(), nullptr, true);

        // The filter chain attached to this parser.
        auto filter = hilti::builder::struct_::field("__filter", _hiltiTypeFilter(), nullptr, true);

        // The bytes object storign the not-yet-filtered data.
        auto filter_dec = hilti::builder::struct_::field("__filter_decoded", _hiltiTypeBytes(), nullptr, true);

        // The current position in the non-yet-filtered data (i.e., in
        // __filter_decoded)
        auto filter_cur = hilti::builder::struct_::field("__filter_cur", _hiltiTypeIteratorBytes(), nullptr, true);

        fields.push_back(self);
        fields.push_back(sink);
        fields.push_back(mtype);
        fields.push_back(filter);
        fields.push_back(filter_dec);
        fields.push_back(filter_cur);
    }

    for ( auto f : fields ) {
        if ( util::startsWith(f->id()->name(), "__") )
            f->setCanRemove();
    }

    auto s = hilti::builder::struct_::type(fields, u->location());

    if ( u->id() ) {
        auto uid = cg()->hiltiID(u->id());
        s->setID(uid);

        if ( u->id()->isScoped() && ! cg()->moduleBuilder()->declared(uid) )
            // An externally defined unit. We define it locally again so that we
            // don't need to import the other HILTI module (because we might not
            // have it ...)
            cg()->moduleBuilder()->addType(uid, s);
    }

    return s;
}

shared_ptr<hilti::Expression> ParserBuilder::_allocateParseObject(shared_ptr<Type> unit, bool store_in_self)
{
    auto rt = cg()->hiltiType(unit);
    auto ut = ast::checkedCast<hilti::type::Reference>(rt)->argType();

    auto pobj = store_in_self ? state()->self : cg()->builder()->addTmp("pobj", rt);

    cg()->builder()->addInstruction(pobj, hilti::instruction::struct_::New, hilti::builder::type::create(ut));

    return pobj;
}

void ParserBuilder::_prepareParseObject(const hilti_expression_type_list& params, shared_ptr<hilti::Expression> cur = nullptr, shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> mimetype)
{
    // Initialize the parameter fields.
    auto arg = params.begin();
    auto u = state()->unit;

    for ( auto p : u->parameters() ) {
        assert(arg != params.end());
        auto field = hilti::builder::string::create(util::fmt("__p_%s", p->id()->name()));
        auto val = cg()->hiltiCoerce(arg->first, arg->second, p->type());
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, field, val);
        ++arg;
    }

    // Initialize non-constant fields with their explicit &defaults.
    for ( auto f : u->flattenedFields() ) {
        auto attr = f->attributes()->lookup("default");

        if ( ! attr )
            continue;

        auto id = hilti::builder::string::create(f->id()->name());
        auto def = cg()->hiltiExpression(attr->value());
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, id, def);
    }

    // Sink variables get special treatment herel they are allocated here.
    // All other fields are left uninitialized.
    for ( auto v : u->variables() ) {
        if ( ast::isA<type::Sink>(v->type()) ) {
            auto id = hilti::builder::string::create(v->id()->name());
            sink = cg()->builder()->addTmp("sink", cg()->hiltiType(v->type()));
            cg()->builder()->addInstruction(sink, hilti::instruction::flow::CallResult,
                                            hilti::builder::id::create("BinPACHilti::sink_new"),
                                            hilti::builder::tuple::create({}));
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, id, sink);
        }
    }

    if ( cur && u->buffering() )
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__input"), cur);

    if ( u->exported() ) {
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__parser"), _hiltiParserDefinition(u));

        if ( mimetype )
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__mimetype"), mimetype);

        if ( sink )
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__sink"), sink);
    }

    _hiltiRunHook(state()->unit, _hookForUnit(state()->unit, "%init"), nullptr, false, nullptr);
}

void ParserBuilder::_finalizeParseObject()
{
    auto unit = state()->unit;

    _hiltiSaveInputPostion();
    _hiltiRunHook(unit, _hookForUnit(unit, "%done"), nullptr, false, nullptr);
    _hiltiUpdateInputPostion();

    // Trim input, we don't need it anymore what we have already parsed.
    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);

    // Clear the parameters to avoid ref-counting cycles.
    for ( auto p : unit->parameters() ) {
        auto name = util::fmt("__p_%s", p->id()->name());
        auto field = hilti::builder::string::create(util::fmt("__p_%s", p->id()->name()));
        cg()->builder()->addInstruction(hilti::instruction::struct_::Unset, state()->self, field);
    }

    // Clear the input positions.
    if ( unit->buffering() ) {
        cg()->builder()->addInstruction(hilti::instruction::struct_::Unset,
                                        state()->self,
                                        hilti::builder::string::create("__input"));

        cg()->builder()->addInstruction(hilti::instruction::struct_::Unset,
                                        state()->self,
                                        hilti::builder::string::create("__cur"));
    }

    // Close all sinks.
    for ( auto v : unit->variables() ) {
        if ( ! ast::isA<type::Sink>(v->type()) )
            continue;

        auto sink = cg()->builder()->addTmp("sink", _hiltiTypeSink());
        cg()->builder()->addInstruction(sink,
                                        hilti::instruction::struct_::Get,
                                        state()->self,
                                        hilti::builder::string::create(v->id()->name()));

        cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                        hilti::builder::id::create("BinPACHilti::sink_close"),
                                        hilti::builder::tuple::create( { sink } ));


        cg()->builder()->addInstruction(hilti::instruction::struct_::Unset,
                                        state()->self,
                                        hilti::builder::string::create(v->id()->name()));
    }

    if ( ! unit->exported() )
        return;

    // Close the filter chain, if anu.
    auto have_filter = cg()->builder()->addTmp("have_filter", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(have_filter,
                                    hilti::instruction::struct_::IsSet,
                                    state()->self,
                                    hilti::builder::string::create("__filter"));

    auto branches = cg()->builder()->addIf(have_filter);
    auto do_filter = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(do_filter);

    auto filter = cg()->builder()->addTmp("filter", _hiltiTypeFilter());
    cg()->builder()->addInstruction(filter,
                                    hilti::instruction::struct_::GetDefault,
                                    state()->self,
                                    hilti::builder::string::create("__filter"),
                                    hilti::builder::reference::createNull());

    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::filter_close"),
                                    hilti::builder::tuple::create( { filter } ));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Unset,
                                    state()->self,
                                    hilti::builder::string::create("__filter"));

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(do_filter);

    cg()->moduleBuilder()->pushBuilder(cont);

    return;
}

void ParserBuilder::_startingProduction(shared_ptr<Production> p, shared_ptr<type::unit::item::Field> field)
{
    cg()->builder()->addComment(util::fmt("Production: %s", util::strtrim(p->render().c_str())));
    cg()->builder()->addComment("");

    if ( cg()->debugLevel() ) {
        _hiltiDebugVerbose(util::fmt("parsing %s", util::strtrim(p->render().c_str())));
        _hiltiDebugShowInput("input", state()->cur);
        _hiltiDebugShowToken("look-ahead", state()->lahead);
        _hiltiDebugShowInput("look-ahead-start", state()->lahstart);
    }

    if ( ! (field && storingValues()) )
        return;

    // Initalize the struct field with the HILTI default value if not already
    // set.
    auto not_set = builder()->addTmp("not_set", hilti::builder::boolean::type(), nullptr, true);
    auto name = hilti::builder::string::create(field->id()->name());
    cg()->builder()->addInstruction(not_set, hilti::instruction::struct_::IsSet, state()->self, name);
    cg()->builder()->addInstruction(not_set, hilti::instruction::boolean::Not, not_set);

    auto branches = cg()->builder()->addIf(not_set);
    auto set_default = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(set_default);
    auto ftype = field->fieldType();
    auto def = cg()->hiltiDefault(ftype, false, false);

    if ( def )
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, name, def);

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(set_default);

    cg()->moduleBuilder()->pushBuilder(cont);

   // Leave builder on stack.
}

void ParserBuilder::_finishedProduction(shared_ptr<Production> p)
{
    cg()->builder()->addComment("");
}

void ParserBuilder::_newValueForField(shared_ptr<Production> p, shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> value)
{
    if ( field ) {

        auto name = field->id()->name();

        if ( value )
            value = cg()->hiltiApplyAttributesToValue(value, field->attributes());

        if ( value ) {
            if ( ast::type::trait::hasTrait<type::trait::Sinkable>(field->fieldType()) )
                cg()->hiltiWriteToSinks(field, value);

            if ( storingValues() ) {
                cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self,
                                                hilti::builder::string::create(name), value);
            }
        }

        else {
            auto ftype = field->fieldType();
            value = cg()->moduleBuilder()->addTmp("field-val", cg()->hiltiType(ftype));
            cg()->builder()->addInstruction(value, hilti::instruction::struct_::Get, state()->self,
                                            hilti::builder::string::create(name));
        }

        if ( cg()->debugLevel() > 0 && ! ast::isA<type::Unit>(field->type()))
            cg()->builder()->addDebugMsg("binpac", util::fmt("%s = %%s", name), value);
    }

    if ( p && p->pgMeta()->for_each ) {
        auto stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, p->pgMeta()->for_each, true, true), p->pgMeta()->for_each, true, value);
        if ( ! stop )
            stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, p->pgMeta()->for_each, true, false), p->pgMeta()->for_each, true, value);

        // FIXME: We ignore stop here. Can we pay attention to that here?
    }

    if ( field ) {
        _hiltiSaveInputPostion();
        hiltiRunFieldHooks(field);
        _hiltiUpdateInputPostion();
    }

    _last_parsed_value = value;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiParserDefinition(shared_ptr<type::Unit> unit)
{
    string name = util::fmt("__binpac_parser_%s", unit->id()->name().c_str());

    auto parser = ast::tryCast<hilti::Expression>(cg()->moduleBuilder()->lookupNode("parser-definition", name));

    if ( parser )
        return parser;

    auto t = hilti::builder::reference::type(hilti::builder::type::byName("BinPACHilti::Parser"));
    parser = cg()->moduleBuilder()->addGlobal(name, t);
    cg()->moduleBuilder()->cacheNode("parser-definition", name, parser);
    return parser;
}

void ParserBuilder::_hiltiDebug(const string& msg)
{
    if ( cg()->debugLevel() > 0 )
        cg()->builder()->addDebugMsg("binpac", msg);
}

void ParserBuilder::_hiltiDebugVerbose(const string& msg)
{
    if ( cg()->debugLevel() > 0 )
        cg()->builder()->addDebugMsg("binpac-verbose", string("- ") + msg);
}

void ParserBuilder::_hiltiDebugShowToken(const string& tag, shared_ptr<hilti::Expression> token)
{
    if ( cg()->debugLevel() == 0 )
        return;

    cg()->builder()->addDebugMsg("binpac-verbose", "  * %s is %s", hilti::builder::string::create(tag), token);
}

void ParserBuilder::_hiltiDebugShowInput(const string& tag, shared_ptr<hilti::Expression> cur)
{
    if ( cg()->debugLevel() == 0 )
        return;

    auto next = cg()->builder()->addTmp("next5", _hiltiTypeBytes());
    cg()->builder()->addInstruction(next, hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPACHilti::next_bytes"),
                                    hilti::builder::tuple::create({ cur, hilti::builder::integer::create(5) }));

    string modetag;

    switch ( state()->mode ) {
     case ParserState::DEFAULT:
        modetag = "default";
        break;

     case ParserState::TRY:
        modetag = "try";
        break;

     case ParserState::LAHEAD_REPARSE:
        modetag = "reparse";
        break;

     default:
        internalError("unknown literal mode in _hiltiDebugShowInput");
    }

    auto frozen = cg()->builder()->addTmp("is_frozen", hilti::builder::boolean::type());
    auto mode = hilti::builder::string::create(modetag);

    cg()->builder()->addInstruction(frozen, hilti::instruction::bytes::IsFrozenIterBytes, cur);

    cg()->builder()->addDebugMsg("binpac-verbose", "  * %s is |%s...| (lit-mode: %s; frozen: %s)",
                                 hilti::builder::string::create(tag), next, mode, frozen);
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
    name = util::strreplace(name, "%", "__0x37");
    name = string("__hook_") + name;

    return std::make_pair(local, name);
}

void ParserBuilder::_hiltiDefineHook(shared_ptr<ID> id, shared_ptr<type::unit::Item> item, bool foreach, bool debug, shared_ptr<type::Unit> unit, shared_ptr<Statement> body, shared_ptr<Type> dollardollar, int priority)
{
    auto unit_module = unit->firstParent<Module>();
    auto current_module = cg()->module();

    if ( debug && cg()->debugLevel() == 0 )
        return;

    auto t = _hookName(id->pathAsString());
    auto local = t.first;
    auto name = t.second;

    if ( unit_module->id()->name() != current_module->id()->name() )
        name = util::fmt("%s::%s", unit_module->id()->name(), name);

    hilti::builder::function::parameter_list p = {
        hilti::builder::function::parameter("__self", cg()->hiltiTypeParseObjectRef(unit), false, nullptr),
        hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), false, nullptr)
    };

    if ( dollardollar ) {
        auto parseable = ast::type::tryTrait<type::trait::Parseable>(dollardollar)->fieldType();
        p.push_back(hilti::builder::function::parameter("__dollardollar", cg()->hiltiType(parseable), false, nullptr));
        cg()->hiltiBindDollarDollar(hilti::builder::id::create("__dollardollar"));
    }

    shared_ptr<hilti::Type> rtype = nullptr;

    if ( foreach )
        rtype = hilti::builder::boolean::type();
    else
        rtype = hilti::builder::void_::type();

    cg()->moduleBuilder()->pushHook(name, hilti::builder::function::result(rtype), p, nullptr, priority, 0, false);

    shared_ptr<hilti::builder::BlockBuilder> hook = nullptr;
    shared_ptr<hilti::builder::BlockBuilder> cont = nullptr;

    if ( debug ) {
        // Add guard code to check whether debug output has been enabled at
        // run-time.
        hook = cg()->moduleBuilder()->newBuilder("debug_hook");
        cont = cg()->moduleBuilder()->newBuilder("debug_cont");
        auto dbg = cg()->builder()->addTmp("dbg", hilti::builder::boolean::type());

        cg()->builder()->addInstruction(dbg, hilti::instruction::flow::CallResult,
                                        hilti::builder::id::create("BinPACHilti::debugging_enabled"),
                                        hilti::builder::tuple::create({}));

        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, dbg, hook->block(), cont->block());

        cg()->moduleBuilder()->pushBuilder(hook);
    }

    pushState(std::make_shared<ParserState>(unit));

    auto msg = util::fmt("- executing hook %s@%s", id->pathAsString(), string(body->location()));
    _hiltiDebugVerbose(msg);

    // TODO: We disable hooks for the item to avoid cycles when the code
    // triggers the same hook again. However, this doesn't catch assigments
    // made from functions called by a hook, but there's no way to catch
    // these statically and adding a dynamic check to every hooks seems
    // overkill. Not sure what to do ...

    if ( item )
        item->disableHooks();

    cg()->hiltiStatement(body);

    if ( item )
        item->enableHooks();

    if ( dollardollar )
        cg()->hiltiUnbindDollarDollar();

    popState();

    if ( hook ) {
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
        cg()->moduleBuilder()->popBuilder(hook);
        cg()->moduleBuilder()->pushBuilder(cont); // Leave on stack.
    }

    cg()->moduleBuilder()->popHook();
}

shared_ptr<hilti::Expression> ParserBuilder::hiltiSelf()
{
    return state()->self;
}

shared_ptr<hilti::Expression> ParserBuilder::hiltiCookie()
{
    return state()->cookie;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiRunHook(shared_ptr<binpac::type::Unit> unit, shared_ptr<ID> id, shared_ptr<type::unit::Item> item, bool foreach, shared_ptr<hilti::Expression> dollardollar)
{
    if ( item && ! item->hooksEnabled() ) {
        if ( foreach )
            std::make_shared<expression::Constant>(std::make_shared<constant::Bool>(false));
        else
            return nullptr;
    }

    auto msg = util::fmt("- triggering hook %s", id->pathAsString());
    _hiltiDebugVerbose(msg);

    auto t = _hookName(id->pathAsString());
    auto local = t.first;
    auto name = t.second;

    // TODO: Don't need "local" anymore I believe.

    auto unit_module = unit->firstParent<Module>();
    assert(unit_module);

    if ( unit_module->id()->name() != cg()->module()->id()->name() )
        name = util::fmt("%s::%s", unit_module->id()->name(), name);

    // Declare the hook if we don't have done that yet.
    if ( ! cg()->moduleBuilder()->lookupNode("hook", name) ) {
        hilti::builder::function::parameter_list p = {
            hilti::builder::function::parameter("__self", cg()->hiltiTypeParseObjectRef(state()->unit), false, nullptr),
            hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), false, nullptr)
        };

        if ( dollardollar )
            p.push_back(hilti::builder::function::parameter("__dollardollar", dollardollar->type(), false, nullptr));

        shared_ptr<hilti::Type> rtype = nullptr;

        if ( foreach )
            rtype = hilti::builder::boolean::type();
        else
            rtype = hilti::builder::void_::type();

        auto hook = cg()->moduleBuilder()->declareHook(name, hilti::builder::function::result(rtype), p);

        cg()->moduleBuilder()->cacheNode("hook", name, hook);
    }

    // Run the hook.
    hilti::builder::tuple::element_list args = { state()->self, state()->cookie };

    if ( dollardollar )
        args.push_back(dollardollar);

    shared_ptr<hilti::Expression> result;

    if ( ! foreach ) {
        cg()->builder()->addInstruction(hilti::instruction::hook::Run,
                                        hilti::builder::id::create(name),
                                        hilti::builder::tuple::create(args));
        result = nullptr;
    }

    else {
        auto stop = cg()->moduleBuilder()->addTmp("hook_result", hilti::builder::boolean::type(), hilti::builder::boolean::create(false));
        cg()->builder()->addInstruction(stop,
                                        hilti::instruction::hook::Run,
                                        hilti::builder::id::create(name),
                                        hilti::builder::tuple::create(args));
        result = stop;
    }

    return result;
}

shared_ptr<binpac::ID> ParserBuilder::_hookForItem(shared_ptr<type::Unit> unit, shared_ptr<type::unit::Item> item, bool foreach, bool private_)
{
    string fe = foreach ? "%foreach" : "";
    string pr = private_ ? util::fmt("%%intern%%%p", item.get()) : "%extern";

    auto id = util::fmt("%s::%s::%s%s%s", cg()->moduleBuilder()->module()->id()->name(), unit->id()->name(), item->id()->name(), fe, pr);
    return std::make_shared<binpac::ID>(id);
}

shared_ptr<binpac::ID> ParserBuilder::_hookForUnit(shared_ptr<type::Unit> unit, const string& name)
{
    auto id = util::fmt("%s::%s::%s", cg()->moduleBuilder()->module()->id()->name(), unit->id()->name(), name);
    return std::make_shared<binpac::ID>(id);
}

void ParserBuilder::_hiltiGetLookAhead(shared_ptr<Production> prod, const std::list<shared_ptr<production::Terminal>>& terms, bool must_find)
{
    assert(terms.size());

    std::list<shared_ptr<production::Terminal>> regexps;
    std::list<shared_ptr<production::Terminal>> other;

    string lahs;
    string id_name;

    bool first = true;

    for ( auto l : terms ) {
        if ( ast::isA<type::RegExp>(l->type()) )
            regexps.push_back(l);
        else
            other.push_back(l);

        if ( ! first )
            lahs += ", ";

        lahs += util::fmt("%s (id %d)", util::strtrim(l->renderTerminal()), l->tokenID());
        id_name += util::fmt("%s_%d", id_name, l->tokenID());

        first = false;
    }

    cg()->builder()->addComment(util::fmt("Searching for look-ahead tokenss: %s", lahs));
    _hiltiDebugVerbose(util::fmt("fetching look-ahead out of tokens {%s}", lahs));

    auto done = cg()->moduleBuilder()->newBuilder("get-lah-done");
    auto loop = cg()->moduleBuilder()->newBuilder("get-lah-loop");

    auto cur = state()->cur;
    auto token = cg()->moduleBuilder()->addTmp("token", _hiltiTypeLookAhead(), nullptr, true);
    auto value = cg()->moduleBuilder()->addTmp("data", _hiltiTypeBytes());
    auto ncur = cg()->moduleBuilder()->addTmp("ncur", _hiltiTypeIteratorBytes());

    cg()->builder()->addInstruction(ncur, hilti::instruction::operator_::Assign, cur);

    // We handle regexps literals jointly first by matching them all in
    // parallel.

    shared_ptr<hilti::Expression> mstate = nullptr;

    if ( regexps.size() )
        mstate = _hiltiMatchTokenInit(id_name, regexps);

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    cg()->moduleBuilder()->pushBuilder(loop);

    shared_ptr<hilti::builder::BlockBuilder> re_done = nullptr;

    if ( regexps.size() ) {
        re_done = cg()->moduleBuilder()->newBuilder("lahead-regexp-done");

        auto mresult = _hiltiMatchTokenAdvance(mstate);
        cg()->builder()->addInstruction(token, hilti::instruction::tuple::Index, mresult, hilti::builder::integer::create(0));

        auto found = cg()->moduleBuilder()->pushBuilder("lahead-regexp-found");
        cg()->builder()->addInstruction(ncur, hilti::instruction::tuple::Index, mresult, hilti::builder::integer::create(1));
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, re_done->block());
        cg()->moduleBuilder()->popBuilder(found);

        hilti::builder::BlockBuilder::case_list cases;

        for ( auto r : regexps ) {
            auto tid = hilti::builder::integer::create(r->tokenID());
            cases.push_back(std::make_pair(tid, found));
        }

        auto default_ = _hiltiAddMatchTokenErrorCases(prod, &cases, loop, regexps, re_done);
        cg()->builder()->addSwitch(token, default_, cases);
    }

    if ( re_done )
        cg()->moduleBuilder()->pushBuilder(re_done);

    // Now iterate through the non-regexps.

    shared_ptr<hilti::Expression> try_cur = nullptr;
    shared_ptr<hilti::Expression> eod = nullptr;

    if ( other.size() ) {
        try_cur = cg()->moduleBuilder()->addTmp("try_cur", _hiltiTypeIteratorBytes());
        eod = cg()->builder()->addTmp("eod", _hiltiTypeIteratorBytes());
        cg()->builder()->addInstruction(eod, hilti::instruction::iterBytes::End, state()->data);
    }

    for ( auto t : other ) {

        auto l = ast::tryCast<production::Literal>(t);

        if ( ! l )
            continue;

        cg()->builder()->addComment(util::fmt("Looking for non-regexp token %s ...", l->render()));

        _cur_literal = l;

        cg()->builder()->addInstruction(try_cur, hilti::instruction::operator_::Assign, cur);

        pushState(state()->clone());

        state()->mode = ParserState::TRY;
        state()->cur = try_cur;

        auto field = l->pgMeta()->field;
        assert(field);

        shared_ptr<hilti::Expression> value;
        processOne(l->literal(), &value, field);

        // If the cur isn't set to end, we have a match.
        auto cond = cg()->moduleBuilder()->addTmp("cond", hilti::builder::boolean::type());
        cg()->builder()->addInstruction(cond, hilti::instruction::operator_::Unequal, try_cur, eod);
        auto branches = cg()->builder()->addIf(cond);
        auto found = std::get<0>(branches);
        auto done = std::get<1>(branches);

        //

        cg()->moduleBuilder()->pushBuilder(found);

        // We take the match if it's longer than what we already have.
        auto old_len = cg()->moduleBuilder()->addTmp("old_len", hilti::builder::integer::type(64));
        auto new_len = cg()->moduleBuilder()->addTmp("new_len", hilti::builder::integer::type(64));

        cg()->builder()->addInstruction(old_len, hilti::instruction::bytes::Diff, cur, ncur);
        cg()->builder()->addInstruction(new_len, hilti::instruction::bytes::Diff, cur, try_cur);

        // If same length, it's ambigious and we abort parsing.
        cg()->builder()->addInstruction(cond, hilti::instruction::integer::Equal, new_len, old_len);

        auto blocks = cg()->builder()->addIf(cond);
        auto ambigious = std::get<0>(blocks);
        auto non_ambigious = std::get<1>(blocks);

        cg()->moduleBuilder()->pushBuilder(ambigious);
        _hiltiParseError("ambigious look-ahead tokens");
        cg()->moduleBuilder()->popBuilder(ambigious);

        cg()->moduleBuilder()->pushBuilder(non_ambigious);

        cg()->builder()->addInstruction(cond, hilti::instruction::integer::Ugt, new_len, old_len);
        auto new_token = cg()->moduleBuilder()->newBuilder("new_token");

        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, new_token->block(), done->block());

        cg()->moduleBuilder()->pushBuilder(new_token);
        auto token_id = hilti::builder::integer::create(l->tokenID());
        cg()->builder()->addInstruction(token, hilti::instruction::operator_::Assign, token_id);
        cg()->builder()->addInstruction(ncur, hilti::instruction::operator_::Assign, try_cur);
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
        cg()->moduleBuilder()->popBuilder(new_token);

        cg()->moduleBuilder()->popBuilder(found);

        //

        cg()->moduleBuilder()->pushBuilder(done); // Leave on stack.

        popState();

        _cur_literal = nullptr;
    }

    // Done checking all the possible token, see if we found the right one.

    auto found_lah = cg()->moduleBuilder()->pushBuilder("get-lah-found");
    cg()->builder()->addInstruction(state()->lahead, hilti::instruction::operator_::Assign, token);
    cg()->builder()->addInstruction(state()->lahstart, hilti::instruction::operator_::Assign, state()->cur);
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::Assign, ncur);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(found_lah);

    hilti::builder::BlockBuilder::case_list cases;

    for ( auto l : terms ) {
        auto tid = hilti::builder::integer::create(l->tokenID());
        cases.push_back(std::make_pair(tid, found_lah));
    }

    auto default_ = _hiltiAddMatchTokenErrorCases(prod, &cases, nullptr, terms, (must_find ? nullptr : done));
    cg()->builder()->addSwitch(token, default_, cases);

    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(done);

    if ( cg()->debugLevel() > 0 )
        cg()->builder()->addDebugMsg("binpac-verbose", "- new look-ahead is %s", state()->lahead);

}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiMatchTokenInit(const string& name, const std::list<shared_ptr<production::Terminal>>& terms)
{
    auto mstate = cg()->builder()->addTmp("match", _hiltiTypeMatchTokenState());

    auto glob = cg()->moduleBuilder()->lookupNode("match-token", name);

    if ( ! glob ) {
        std::list<std::pair<string, string>> tokens;

        for ( auto t : terms ) {
            // FIXME: Really shouldn't have to do this cast here. Should we
            // pass in a list of Literals instead?
            auto l = ast::checkedCast<production::Literal>(t);
            for ( auto p : l->patterns() )
                tokens.push_back(std::make_pair(util::fmt("%s{#%d}", p.first, l->tokenID()), ""));
        }

        auto op = hilti::builder::regexp::create(tokens);
        auto rty = hilti::builder::reference::type(hilti::builder::regexp::type({"&nosub"}));
        glob = cg()->moduleBuilder()->addGlobal(hilti::builder::id::node(name), rty, op);

        cg()->moduleBuilder()->cacheNode("match-token", name, glob);
    }

    auto pattern = ast::checkedCast<hilti::Expression>(glob);

    cg()->builder()->addInstruction(state()->lahstart, hilti::instruction::operator_::Assign, state()->cur); // Record start position.
    cg()->builder()->addInstruction(mstate, hilti::instruction::regexp::MatchTokenInit, pattern);

    return mstate;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiMatchTokenAdvance(shared_ptr<hilti::Expression> mstate)
{
    auto mresult = cg()->builder()->addTmp("match_result", _hiltiTypeMatchResult());
    auto eob = cg()->builder()->addTmp("eob", _hiltiTypeIteratorBytes());
    cg()->builder()->addInstruction(eob, hilti::instruction::iterBytes::End, state()->data);
    cg()->builder()->addInstruction(mresult, hilti::instruction::regexp::MatchTokenAdvanceBytes, mstate, state()->cur, eob);
    return mresult;
}

shared_ptr<hilti::builder::BlockBuilder> ParserBuilder::_hiltiAddMatchTokenErrorCases(shared_ptr<Production> prod,
                                                                             hilti::builder::BlockBuilder::case_list* cases,
                                                                             shared_ptr<hilti::builder::BlockBuilder> repeat,
                                                                             std::list<shared_ptr<production::Terminal>> expected,
                                                                             shared_ptr<hilti::builder::BlockBuilder> cont
                                                                             )
{
    shared_ptr<hilti::builder::BlockBuilder> b = nullptr;

    // Not found.
    if ( ! cont ) {
        b = cg()->moduleBuilder()->cacheBlockBuilder("not-found", [&] () {
            _hiltiParseError("expected symbol(s) not found");
        });
    }

    else {
        b = cg()->moduleBuilder()->pushBuilder("not-found");
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
        cg()->moduleBuilder()->popBuilder(b);
    }

    cases->push_back(std::make_pair(_hiltiLookAheadNone(), b));

    // Insufficient input.
    if ( repeat ) {
        b = cg()->moduleBuilder()->cacheBlockBuilder("insufficient-input", [&] () {
            _hiltiYieldAndTryAgain(prod, repeat);
        });
    }

    else {
        b = cg()->moduleBuilder()->cacheBlockBuilder("insufficient-input-unexpected", [&] () {
            cg()->builder()->addInternalError("unexpected yield");
        });
    }

    cases->push_back(std::make_pair(hilti::builder::integer::create(-1), b));

    // Internal error: Unexpected token found (default case for the switch).
    b = cg()->moduleBuilder()->cacheBlockBuilder("match-token-error", [&] () {
        cg()->builder()->addInternalError("unexpected symbol found");
    });

    return b;
}

void ParserBuilder::_hiltiParseError(const string& msg)
{
    _hiltiDebugVerbose("raising parse error");
    cg()->builder()->addThrow("BinPACHilti::ParseError", hilti::builder::string::create(msg));
}

void ParserBuilder::_hiltiYieldAndTryAgain(shared_ptr<Production> prod, shared_ptr<hilti::builder::BlockBuilder> cont)
{
    if ( ! prod->eodOk() ) {
        _hiltiInsufficientInputHandler();
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    }

    else {
        auto eod = cg()->moduleBuilder()->pushBuilder("eod-ok");
        auto result = hilti::builder::tuple::create({state()->cur, state()->lahead, state()->lahstart});
        cg()->builder()->addInstruction(hilti::instruction::flow::ReturnResult, result);
        cg()->moduleBuilder()->popBuilder();

        auto at_eod = _hiltiInsufficientInputHandler(true);
        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, at_eod, eod->block(), cont->block());
    }
}

shared_ptr<hilti::Expression> ParserBuilder::hiltiUnpack(shared_ptr<Type> target_type,
                                                         shared_ptr<hilti::Expression> op1,
                                                         shared_ptr<hilti::Expression> op2,
                                                         shared_ptr<hilti::Expression> op3,
                                                         unpack_callback callback)
{
    auto parse = cg()->moduleBuilder()->newBuilder("parse");
    auto cont = cg()->moduleBuilder()->newBuilder("cont");
    auto yield = cg()->moduleBuilder()->newBuilder("yield");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, parse->block());

    cg()->moduleBuilder()->pushBuilder(parse);

    auto rtype = hilti::builder::tuple::type({ cg()->hiltiType(target_type), _hiltiTypeIteratorBytes() });
    auto result = cg()->builder()->addTmp("unpacked", rtype, nullptr, true);
    auto result_val = cg()->builder()->addTmp("unpacked_val", cg()->hiltiType(target_type), nullptr, true);
    auto blocked = cg()->builder()->addTmp("blocked", hilti::builder::boolean::type());

    cg()->builder()->addInstruction(blocked, hilti::instruction::operator_::Assign, hilti::builder::boolean::create(false));

    cg()->builder()->beginTryCatch();

    auto end = cg()->builder()->addTmp("unpack-end", _hiltiTypeIteratorBytes(), nullptr, true);
    cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);

    auto iters = hilti::builder::tuple::create({ state()->cur, end });
    cg()->builder()->addInstruction(result, hilti::instruction::operator_::Unpack, iters, op2, op3);
    cg()->builder()->addInstruction(result_val, hilti::instruction::tuple::Index, result, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::tuple::Index, result, hilti::builder::integer::create(1));

    //

    cg()->builder()->pushCatch(hilti::builder::reference::type(hilti::builder::type::byName("Hilti::WouldBlock")),
                               hilti::builder::id::node("e"));
    cg()->builder()->addInstruction(blocked, hilti::instruction::operator_::Assign, hilti::builder::boolean::create(true));
    cg()->builder()->popCatch();

    if ( state()->mode != ParserState::TRY ) {
        cg()->builder()->pushCatchAll();
        cg()->builder()->addThrow("BinPACHilti::ParseError", hilti::builder::string::create("unpack failed"));
        cg()->builder()->popCatchAll();
    }

    cg()->builder()->endTryCatch();

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, blocked, yield->block(), cont->block());

    cg()->moduleBuilder()->popBuilder(parse);

    //

    cg()->moduleBuilder()->pushBuilder(yield);

    if ( callback )
        callback();

    _hiltiInsufficientInputHandler(false);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, parse->block());

    cg()->moduleBuilder()->popBuilder(yield);

    cg()->moduleBuilder()->pushBuilder(cont);

    return result_val;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiInsufficientInputHandler(bool eod_ok, shared_ptr<hilti::Expression> iter)
{
    shared_ptr<hilti::Expression> result = nullptr;

    auto frozen = cg()->builder()->addTmp("frozen", hilti::builder::boolean::type(), nullptr, true);
    auto resume = cg()->moduleBuilder()->newBuilder("resume");

    auto suspend = cg()->moduleBuilder()->pushBuilder("suspend");
    _hiltiDebugVerbose("out of input, yielding ...");
    cg()->builder()->addInstruction(hilti::instruction::flow::YieldUntil, state()->data);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, resume->block());
    cg()->moduleBuilder()->popBuilder(suspend);

    cg()->builder()->addInstruction(frozen, hilti::instruction::bytes::IsFrozenBytes, state()->data);

    if ( eod_ok ) {
        _hiltiDebugVerbose("insufficient input (but end-of-data is ok here)");
        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, frozen, resume->block(), suspend->block());
        result = frozen;
    }

    else {
        _hiltiDebugVerbose("insufficient input (end-of-data is not ok here)");

        auto error = cg()->moduleBuilder()->cacheBlockBuilder("error", [&] () {
            _hiltiParseError("insufficient input");
        });

        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, frozen, error->block(), suspend->block());
        result = hilti::builder::boolean::create(false);
    }

    cg()->moduleBuilder()->pushBuilder(resume);

    _hiltiFilterInput(true);

    // Leave on stack.

    return result;
}

shared_ptr<binpac::Expression> ParserBuilder::_fieldByteOrder(shared_ptr<type::unit::item::Field> field, shared_ptr<type::Unit> unit)
{
    shared_ptr<binpac::Expression> order = nullptr;

    auto module = unit->firstParent<Module>();
    assert(module);

    // See if the module, unit, or field has a byte order property defined.
    auto module_order = module->property("byteorder");
    auto unit_order = unit->property("byteorder");
    auto field_order = field->attributes()->lookup("byteorder");

    if ( field_order )
        order = field_order->value();

    else if ( unit_order )
        order = unit_order->property()->value();

    else if ( module_order )
        order = module_order->value();

    return order;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiIntUnpackFormat(int width, bool signed_, shared_ptr<binpac::Expression> byteorder)
{
    auto hltbo = byteorder ? cg()->hiltiExpression(byteorder) : hilti::builder::id::create("BinPAC::ByteOrder::Big");

    string big, little, host;

    switch ( width ) {
     case 8:
        if ( signed_ ) { little = "Int8Little"; big = "Int8Big"; host = "Int8"; }
        else           { little = "UInt8Little"; big = "UInt8Big"; host = "UInt8"; }
        break;

     case 16:
        if ( signed_ ) { little = "Int16Little"; big = "Int16Big"; host = "Int16"; }
        else           { little = "UInt16Little"; big = "UInt16Big"; host = "UInt16"; }
        break;

     case 32:
        if ( signed_ ) { little = "Int32Little"; big = "Int32Big"; host = "Int32"; }
        else           { little = "UInt32Little"; big = "UInt32Big"; host = "UInt32"; }
        break;

     case 64:
        if ( signed_ ) { little = "Int64Little"; big = "Int64Big"; host = "Int64"; }
        else           { little = "UInt64Little"; big = "UInt64Big"; host = "UInt64"; }
        break;

     default:
        internalError("unsupported bitwidth in _hiltiIntUnpackFormat()");
    }

    auto t1 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Little"),
        hilti::builder::id::create(string("Hilti::Packed::") + little) });

    auto t2 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Big"),
        hilti::builder::id::create(string("Hilti::Packed::") + big) });

    auto t3 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Host"),
        hilti::builder::id::create(string("Hilti::Packed::") + host) });

    auto tuple = hilti::builder::tuple::create({ t1, t2, t3 });
    auto result = cg()->moduleBuilder()->addTmp("fmt", hilti::builder::type::byName("Hilti::Packed"));
    cg()->builder()->addInstruction(result, hilti::instruction::Misc::SelectValue, hltbo, tuple);

    return result;
}

void ParserBuilder::disableStoringValues()
{
    --_store_values;
}

void ParserBuilder::enableStoringValues()
{
    ++_store_values;
}

bool ParserBuilder::storingValues()
{
    return _store_values > 0;
}

void ParserBuilder::_hiltiSaveInputPostion()
{
    if ( state()->unit->buffering() )
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__cur"), state()->cur);
}

void ParserBuilder::_hiltiUpdateInputPostion()
{
    if ( state()->unit->buffering() )
        cg()->builder()->addInstruction(state()->cur, hilti::instruction::struct_::Get, state()->self, hilti::builder::string::create("__cur"));
}

void ParserBuilder::_hiltiFilterInput(bool resume)
{
    if ( ! state()->unit->exported() )
        return;

    auto have_filter = cg()->builder()->addTmp("have_filter", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(have_filter,
                                    hilti::instruction::struct_::IsSet,
                                    state()->self,
                                    hilti::builder::string::create("__filter"));

    auto branches = cg()->builder()->addIf(have_filter);
    auto do_filter = std::get<0>(branches);
    auto done = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(do_filter);

    cg()->builder()->addComment("Passing input through filter chain");

    auto filter = cg()->builder()->addTmp("filter", _hiltiTypeFilter());
    auto encoded = cg()->builder()->addTmp("encoded", _hiltiTypeBytes());
    auto decoded = cg()->builder()->addTmp("decoded", _hiltiTypeBytes());
    auto filter_decoded = cg()->builder()->addTmp("filter_decoded", _hiltiTypeBytes());
    auto filter_cur = cg()->builder()->addTmp("filter_cur", _hiltiTypeIteratorBytes());
    auto begin = cg()->builder()->addTmp("begin", _hiltiTypeIteratorBytes());
    auto end = cg()->builder()->addTmp("end", _hiltiTypeIteratorBytes());
    auto len = cg()->builder()->addTmp("len", hilti::builder::integer::type(64));
    auto is_frozen = cg()->builder()->addTmp("is_frozen", hilti::builder::boolean::type());

    cg()->builder()->addInstruction(filter,
                                    hilti::instruction::struct_::Get,
                                    state()->self,
                                    hilti::builder::string::create("__filter"));

    if ( ! resume ) {
        //cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);
        cg()->builder()->addInstruction(encoded, hilti::instruction::bytes::Sub, state()->cur, end);
        cg()->builder()->addInstruction(is_frozen, hilti::instruction::bytes::IsFrozenBytes, state()->data);
    }

    else {
        cg()->builder()->addInstruction(filter_cur,
                                        hilti::instruction::struct_::Get,
                                        state()->self,
                                        hilti::builder::string::create("__filter_cur"));

        //cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, filter_decoded);
        cg()->builder()->addInstruction(encoded, hilti::instruction::bytes::Sub, filter_cur, end);

        cg()->builder()->addInstruction(filter_decoded,
                                        hilti::instruction::struct_::Get,
                                        state()->self,
                                        hilti::builder::string::create("__filter_decoded"));

        cg()->builder()->addInstruction(is_frozen, hilti::instruction::bytes::IsFrozenBytes, filter_decoded);
    }

    cg()->builder()->addInstruction(decoded,
                                    hilti::instruction::flow::CallResult,
                                    hilti::builder::id::create("BinPACHilti::filter_decode"),
                                    hilti::builder::tuple::create({ filter, encoded }));


    if ( ! resume ) {
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                        state()->self,
                                        hilti::builder::string::create("__filter_cur"),
                                        state()->cur);

        cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                        state()->self,
                                        hilti::builder::string::create("__filter_decoded"),
                                        state()->data);

        cg()->builder()->addInstruction(begin, hilti::instruction::iterBytes::Begin, decoded);

        cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::Assign, begin);
        cg()->builder()->addInstruction(state()->data, hilti::instruction::operator_::Assign, decoded);

        if ( state()->unit->buffering() ) {
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                            state()->self,
                                            hilti::builder::string::create("__input"),
                                            begin);

            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, // TODO: Do we need this one?
                                            state()->self,
                                            hilti::builder::string::create("__cur"),
                                            begin);
        }

        cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, begin);
    }

    else { // Resuming.
        cg()->builder()->addInstruction(hilti::instruction::bytes::Append, state()->data, decoded);
    }


    cg()->builder()->addInstruction(filter_cur,
                                    hilti::instruction::struct_::Get,
                                    state()->self,
                                    hilti::builder::string::create("__filter_cur"));

    cg()->builder()->addInstruction(len, hilti::instruction::bytes::Length, encoded);
    cg()->builder()->addInstruction(filter_cur, hilti::instruction::iterBytes::IncrBy, filter_cur, len);

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    state()->self,
                                    hilti::builder::string::create("__filter_cur"),
                                    filter_cur);

    cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, filter_cur);

    // Freeze the decoded data if the input data is frozen.
    branches = cg()->builder()->addIf(is_frozen);
    auto do_freeze = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(do_freeze);
    cg()->builder()->addInstruction(hilti::instruction::bytes::Freeze, state()->data);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(do_freeze);

    cg()->moduleBuilder()->pushBuilder(cont);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(cont);

    cg()->moduleBuilder()->pushBuilder(done);

    // Leave on stack.
}


void ParserBuilder::hiltiWriteToSinks(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> data)
{
    // Pass into any attached sinks.
    for ( auto s: field->sinks() ) {
        auto sink = cg()->hiltiExpression(s);
        hiltiWriteToSink(sink, data);
    }
}

void ParserBuilder::hiltiWriteToSink(shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> data)
{
    cg()->builder()->addInstruction(hilti::instruction::flow::CallVoid,
                                    hilti::builder::id::create("BinPACHilti::sink_write"),
                                    hilti::builder::tuple::create( { sink, data, cg()->hiltiCookie() } ));
}

////////// Visit methods.

void ParserBuilder::visit(expression::Ctor* c)
{
    auto field = arg1();
    assert(field);

    shared_ptr<hilti::Expression> value;
    processOne(c->ctor(), &value, field);

    setResult(value);
}

void ParserBuilder::visit(expression::Constant* c)
{
    auto field = arg1();
    assert(field);

    shared_ptr<hilti::Expression> value;
    processOne(c->constant(), &value, field);

    setResult(value);
}

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
    auto field = arg1();
    assert(field);

    cg()->builder()->addComment(util::fmt("Integer constant: %s", field->id()->name()));

    if ( state()->mode == ParserState::LAHEAD_REPARSE ) {
        auto value = cg()->hiltiExpression(std::make_shared<expression::Constant>(i->sharedPtr<constant::Integer>()));
        setResult(value);
        return;
    }

    // We parse the field normally according to its type, and then compare
    // the value against the constant we expec.t
    shared_ptr<hilti::Expression> value;
    processOne(field->type(), &value, field);

    auto expr = std::make_shared<expression::Constant>(i->sharedPtr<constant::Integer>());
    auto mismatch = cg()->builder()->addTmp("mismatch", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(mismatch, hilti::instruction::operator_::Unequal, value, cg()->hiltiExpression(expr));

    auto branches = cg()->builder()->addIf(mismatch);
    auto error = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(error);

    if ( state()->mode == ParserState::TRY )
        cg()->builder()->addInstruction(state()->cur, hilti::instruction::iterBytes::End, state()->data);

    else
        _hiltiParseError(util::fmt("integer constant expected (%s)", i->render()));

    cg()->moduleBuilder()->popBuilder(error);

    cg()->moduleBuilder()->pushBuilder(cont);

    setResult(value);
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
    if ( state()->mode == ParserState::LAHEAD_REPARSE ) {
        auto value = cg()->moduleBuilder()->addTmp("data", _hiltiTypeBytes());
        cg()->builder()->addInstruction(value, hilti::instruction::bytes::Sub, state()->lahstart, state()->cur);
        setResult(value);
        return;
    }

    auto field = arg1();
    assert(field);
    assert(_cur_literal);

    auto lit = _cur_literal;

    cg()->builder()->addComment(util::fmt("RegExp ctor: %s", field->id()->name()));

    auto done = cg()->moduleBuilder()->newBuilder("done-regexp");
    auto loop = cg()->moduleBuilder()->newBuilder("regexp-loop");

    auto token_id = hilti::builder::integer::create(lit->tokenID());
    auto name = util::fmt("__literal_%d", lit->tokenID());

    auto symbol = cg()->moduleBuilder()->addTmp("token", _hiltiTypeLookAhead(), nullptr, true);
    auto value = cg()->moduleBuilder()->addTmp("data", _hiltiTypeBytes());
    auto ncur = cg()->moduleBuilder()->addTmp("ncur", _hiltiTypeIteratorBytes());

    auto mstate = _hiltiMatchTokenInit(name, { lit->sharedPtr<production::Literal>() });
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    cg()->moduleBuilder()->pushBuilder(loop);

    auto mresult = _hiltiMatchTokenAdvance(mstate);

    cg()->builder()->addInstruction(symbol, hilti::instruction::tuple::Index, mresult, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(ncur, hilti::instruction::tuple::Index, mresult, hilti::builder::integer::create(1));

    auto found_lit = cg()->moduleBuilder()->pushBuilder("found-literal");
    cg()->builder()->addInstruction(value, hilti::instruction::bytes::Sub, state()->cur, ncur);
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::Assign, ncur); // Move input position.
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(found_lit);

    hilti::builder::BlockBuilder::case_list cases;
    cases.push_back(std::make_pair(token_id, found_lit));

    auto default_ = _hiltiAddMatchTokenErrorCases(lit->sharedPtr<Production>(), &cases, loop, { lit->sharedPtr<production::Literal>() });

    cg()->builder()->addSwitch(symbol, default_, cases);

    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(done);

    cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, mresult);
    cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, mstate);

    setResult(value);
}

void ParserBuilder::visit(production::Boolean* b)
{
}

void ParserBuilder::visit(production::ChildGrammar* c)
{
    auto field = c->pgMeta()->field;
    assert(field);

    bool in_container = (c->container() != nullptr);

    _startingProduction(c->sharedPtr<Production>(), (in_container ? nullptr : field));

    auto child = c->childType();
    auto subself = _allocateParseObject(child, false);

    hilti_expression_type_list params;

    for ( auto p : util::zip2(field->parameters(), child->parameters()) ) {
        auto t = p.second->type();
        params.push_back(std::make_pair(cg()->hiltiExpression(p.first, t), t));
    }

    auto pstate = std::make_shared<ParserState>(child, subself, state()->data, state()->cur, state()->lahead, state()->lahstart, state()->cookie);
    pushState(pstate);

    _prepareParseObject(params, state()->cur);

    auto child_result = cg()->builder()->addTmp("presult", _hiltiTypeParseResult());
    auto child_func = cg()->hiltiParseFunction(child);

    if ( cg()->debugLevel() > 0 ) {
        if ( ! field->anonymous() )
            _hiltiDebug(field->id()->name());
        else
            _hiltiDebug(field->type()->render());

        cg()->builder()->debugPushIndent();
    }

    cg()->builder()->addInstruction(child_result, hilti::instruction::flow::CallResult, child_func, state()->hiltiArguments());

    if ( cg()->debugLevel() > 0 )
        cg()->builder()->debugPopIndent();

    popState();

    cg()->builder()->addInstruction(state()->cur, hilti::instruction::tuple::Index, child_result, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(state()->lahead, hilti::instruction::tuple::Index, child_result, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(state()->lahstart, hilti::instruction::tuple::Index, child_result, hilti::builder::integer::create(2));

    _newValueForField(c->sharedPtr<Production>(), (in_container ? nullptr : field), subself);

    _finishedProduction(c->sharedPtr<Production>());

    setResult(subself);
}

void ParserBuilder::visit(production::Enclosure* c)
{
    auto field = c->pgMeta()->field;
    assert(field);

    // We support enclosures only for containers currently.
    assert(ast::isA<type::unit::item::field::Container>(field));

    _startingProduction(c->sharedPtr<Production>(), field);

    parse(c->child());

    _newValueForField(c->sharedPtr<Production>(), field, nullptr);
    _finishedProduction(c->sharedPtr<Production>());
}

void ParserBuilder::visit(production::Counter* c)
{
    auto field = c->pgMeta()->field;
    assert(field);

    _startingProduction(c->sharedPtr<Production>(), field);

    auto i = cg()->builder()->addTmp("count-i", hilti::builder::integer::type(64));
    auto b = cg()->builder()->addTmp("count-bool", hilti::builder::boolean::type());
    auto cont = cg()->moduleBuilder()->newBuilder("count-done");
    auto parse_one = cg()->moduleBuilder()->newBuilder("count-parse-one");
    auto loop = cg()->moduleBuilder()->newBuilder("count-loop");

    auto cnt = cg()->hiltiExpression(c->expression(), std::make_shared<type::Integer>(64, false));
    cg()->builder()->addInstruction(i, hilti::instruction::operator_::Assign, cnt);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    cg()->moduleBuilder()->pushBuilder(loop);
    cg()->builder()->addInstruction(b, hilti::instruction::integer::Sleq, i, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, b, cont->block(), parse_one->block());
    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(parse_one);

    disableStoringValues();
    shared_ptr<hilti::Expression> value;
    parse(c->body(), &value);
    enableStoringValues();

    // Run foreach hook.
    auto stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, field, true, true), field, true, value);
    if ( ! stop )
        stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, field, true, false), field, true, value);

    cg()->builder()->addInstruction(i, hilti::instruction::integer::Decr, i);
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, stop, cont->block(), loop->block());

    cg()->moduleBuilder()->popBuilder(parse_one);

    cg()->moduleBuilder()->pushBuilder(cont);

    _newValueForField(c->sharedPtr<Production>(), field, nullptr);
    _finishedProduction(c->sharedPtr<Production>());

    // Leave builder on stack.
}

void ParserBuilder::visit(production::Epsilon* e)
{
}

void ParserBuilder::visit(production::Literal* l)
{
    auto field = l->pgMeta()->field;
    assert(field);

    _startingProduction(l->sharedPtr<Production>(), field);

    cg()->builder()->addComment(util::fmt("Literal: %s (id %d)", field->id()->name(), l->tokenID()));

    auto ltype = ast::type::checkedTrait<type::trait::Parseable>(l->type())->fieldType();
    auto literal = cg()->moduleBuilder()->addTmp("literal", cg()->hiltiType(ltype));

    // See if we have a look-ahead symbol.
    auto cond = cg()->builder()->addTmp("cond", hilti::builder::boolean::type(), nullptr, true);
    auto equal = cg()->builder()->addInstruction(cond, hilti::instruction::integer::Equal, state()->lahead, _hiltiLookAheadNone());

    auto branches = cg()->builder()->addIfElse(cond);
    auto no_lahead = std::get<0>(branches);
    auto have_lahead = std::get<1>(branches);
    auto done = std::get<2>(branches);

    // If we don't have a look-ahead, parse the literal.

    cg()->moduleBuilder()->pushBuilder(no_lahead);
    cg()->builder()->addComment("No look-ahead symbol pending, parsing literal ...");

    _cur_literal = l->sharedPtr<production::Literal>();
    shared_ptr<hilti::Expression> value;
    processOne(l->literal(), &value, field);
    cg()->builder()->addInstruction(literal, hilti::instruction::operator_::Assign, value);
    _cur_literal = nullptr;

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(no_lahead);

    // If have a look-ahead symbol, its value must match what we expect.

    cg()->moduleBuilder()->pushBuilder(have_lahead);
    cg()->builder()->addComment("Look-ahead symbol pending, checking ...");

    auto wrong_lahead = cg()->moduleBuilder()->pushBuilder("wrong-lahead");
    _hiltiParseError("unexpected look-ahead symbol pending");
    cg()->moduleBuilder()->popBuilder(wrong_lahead);

    auto consume_lahead = cg()->moduleBuilder()->pushBuilder("consume-lahead");

    _hiltiDebugVerbose("consuming look-ahead ...");

    // Still need to get the value for the look-ahead symbol.
    ParserState::LiteralMode old_mode = state()->mode;
    state()->mode = ParserState::LAHEAD_REPARSE;

    _cur_literal = l->sharedPtr<production::Literal>();
    processOne(l->literal(), &value, field);
    cg()->builder()->addInstruction(literal, hilti::instruction::operator_::Assign, value);
    _cur_literal = nullptr;

    state()->mode = old_mode;

    cg()->builder()->addInstruction(state()->lahead, hilti::instruction::operator_::Assign, _hiltiLookAheadNone()); // Clear lahead.
    cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, state()->lahstart);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(consume_lahead);

    auto token_id = hilti::builder::integer::create(l->tokenID());
    cg()->builder()->addInstruction(cond, hilti::instruction::integer::Equal, token_id, state()->lahead);
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, consume_lahead->block(), wrong_lahead->block());

    cg()->moduleBuilder()->popBuilder(have_lahead);

    // Got a value.

    cg()->moduleBuilder()->pushBuilder(done);

    _newValueForField(l->sharedPtr<Production>(), field, literal);
    _finishedProduction(l->sharedPtr<Production>());

    setResult(literal);

    // Leave builder on stack.
}

void ParserBuilder::visit(production::LookAhead* l)
{
    _startingProduction(l->sharedPtr<Production>(), nullptr);

    auto done = cg()->moduleBuilder()->newBuilder("lah-done");

    // If we don't have a look-ahead symbol pending, get one.

    auto need_lah = cg()->builder()->addTmp("need_lah", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(need_lah, hilti::instruction::operator_::Equal, state()->lahead, _hiltiLookAheadNone());

    auto branches = cg()->builder()->addIf(need_lah);
    auto no_lahead = std::get<0>(branches);
    auto have_lahead = std::get<1>(branches);

    cg()->moduleBuilder()->pushBuilder(no_lahead);

    std::list<shared_ptr<production::Terminal>> terms;

    for ( auto l : l->lookAheads().first )
        terms.push_back(l);

    for ( auto l : l->lookAheads().second )
        terms.push_back(l);

    auto defaults = l->defaultAlternatives();
    assert(! (defaults.first && defaults.second));

    bool must_find = (defaults.first && defaults.second);
    _hiltiGetLookAhead(l->sharedPtr<production::LookAhead>(), terms, must_find);

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, have_lahead->block());
    cg()->moduleBuilder()->popBuilder(no_lahead);

    // Block when expected symbol for alternative 1 is found.
    auto found_alt1 = cg()->moduleBuilder()->pushBuilder("found-alt1-lah");
    cg()->builder()->addComment("Look-ahead alternative 1");
    parse(l->alternatives().first);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(found_alt1);

    // Block when expected symbol for alternative 2 is found.
    auto found_alt2 = cg()->moduleBuilder()->pushBuilder("found-alt2-lah");
    cg()->builder()->addComment("Look-ahead alternative 2");
    parse(l->alternatives().second);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(found_alt2);

    // Block when an unexpected look-ahead symbol is found, which shoudln't
    // happen at this point anymore and thus triggers an internal error.
    auto wrong_symbol = cg()->moduleBuilder()->pushBuilder("wrong-lah");
    cg()->builder()->addInternalError("unexpected lahead symbol set");
    cg()->moduleBuilder()->popBuilder(wrong_symbol);

    // Now that we for sure have a lahead symbol pending, make sure it's one
    // of those we expect.

    cg()->moduleBuilder()->pushBuilder(have_lahead);

    hilti::builder::BlockBuilder::case_list cases;

    for ( auto l : l->lookAheads().first ) {
        auto tid = hilti::builder::integer::create(l->tokenID());
        cases.push_back(std::make_pair(tid, found_alt1));
    }

    for ( auto l : l->lookAheads().second ) {
        auto tid = hilti::builder::integer::create(l->tokenID());
        cases.push_back(std::make_pair(tid, found_alt2));
    }

    // See if one of the two is the default alternative to take when no
    // look-ahead symbol is found.

    if ( defaults.first )
        cases.push_back(std::make_pair(_hiltiLookAheadNone(), found_alt1));

    else if ( defaults.second )
        cases.push_back(std::make_pair(_hiltiLookAheadNone(), found_alt2));

    else {
        auto not_found = cg()->moduleBuilder()->pushBuilder("lahead-not-found");
        _hiltiParseError("none of the expected look-ahead tokens found");
        cg()->moduleBuilder()->popBuilder(not_found);

        cases.push_back(std::make_pair(_hiltiLookAheadNone(), not_found));
    }

    cg()->builder()->addSwitch(state()->lahead, wrong_symbol, cases);

    cg()->moduleBuilder()->popBuilder(have_lahead);

    cg()->moduleBuilder()->pushBuilder(done);

    // Leave on stack.

    _finishedProduction(l->sharedPtr<Production>());
}

void ParserBuilder::visit(production::NonTerminal* n)
{
}

void ParserBuilder::visit(production::Sequence* s)
{
    _startingProduction(s->sharedPtr<Production>(), nullptr);

    for ( auto p : s->sequence() )
        parse(p);

    _finishedProduction(s->sharedPtr<Production>());
}

void ParserBuilder::visit(production::Switch* s)
{
    _startingProduction(s->sharedPtr<Production>(), nullptr);

    auto cont = cg()->moduleBuilder()->newBuilder("switch-cont");

    hilti::builder::BlockBuilder::case_list cases;

    // Build the branches.
    for ( auto c : s->alternatives() ) {
        auto builder = cg()->moduleBuilder()->pushBuilder("switch-case");
        parse(c.second);
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
        cg()->moduleBuilder()->popBuilder(builder);

        for ( auto e : c.first ) {
            auto expr = cg()->hiltiExpression(e, s->expression()->type());
            cases.push_back(std::make_pair(expr, builder));
        }
    }

    // Build default branch, raising a parse error if none given.
    auto default_ = cg()->moduleBuilder()->pushBuilder("switch-default");

    if ( s->default_() )
        parse(s->default_());
    else
        _hiltiParseError("no matching switch case");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(default_);

    auto expr = cg()->hiltiExpression(s->expression(), s->expression()->type());
    cg()->builder()->addSwitch(expr, default_, cases);

    cg()->moduleBuilder()->pushBuilder(cont);

    _finishedProduction(s->sharedPtr<Production>());
}

void ParserBuilder::visit(production::Terminal* t)
{
}

void ParserBuilder::visit(production::Variable* v)
{
    auto field = v->pgMeta()->field;
    assert(field);

    _startingProduction(v->sharedPtr<Production>(), field);

    cg()->builder()->addComment(util::fmt("Variable: %s", field->id()->name()));

    shared_ptr<hilti::Expression> value;
    parse(v->type(), &value, field);

    _newValueForField(v->sharedPtr<Production>(), field, value);
    _finishedProduction(v->sharedPtr<Production>());

    setResult(value);
}

void ParserBuilder::visit(production::While* w)
{
    // TODO: Unclear if we need this, but if so, borrow code from
    // production::Until below.
}

#if 0

// Looks like we don't need this, but may reuse the code for the While (if we need that one ...)

void ParserBuilder::visit(production::Until* w)
{
    auto field = w->pgMeta()->field;
    assert(field);

    _startingProduction(w->sharedPtr<Production>(), field);

    auto done = cg()->moduleBuilder()->newBuilder("until-done");
    auto cont = cg()->moduleBuilder()->newBuilder("until-cont");

    auto loop = cg()->moduleBuilder()->pushBuilder("until-loop");

    disableStoringValues();
    shared_ptr<hilti::Expression> value;
    parse(w->body(), &value);
    enableStoringValues();

    auto cond = cg()->hiltiExpression(w->expression(), std::make_shared<type::Bool>());

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, done->block(), cont->block());
    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(cont);

    auto stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, field, true, true), field, true, value);
    if ( ! stop )
        stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, field, true, false), field, true, value);

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, stop, done->block(), loop->block());
    cg()->moduleBuilder()->popBuilder(cont);

    // Run foreach hook.

    cg()->moduleBuilder()->pushBuilder(done);

    _newValueForField(w->sharedPtr<Production>(), field, nullptr);
    _finishedProduction(w->sharedPtr<Production>());

    // Leave builder on stack.
}

#endif

void ParserBuilder::visit(production::Loop* l)
{
    auto field = l->pgMeta()->field;
    assert(field);

    _startingProduction(l->sharedPtr<Production>(), field);

    auto done = cg()->moduleBuilder()->newBuilder("loop-done");
    auto loop = cg()->moduleBuilder()->newBuilder("loop-body");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    cg()->moduleBuilder()->pushBuilder(loop);

    disableStoringValues();
    shared_ptr<hilti::Expression> value;
    parse(l->body(), &value);
    enableStoringValues();

    // Run foreach hook.
    auto stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, field, true, true), field, true, value);
    if ( ! stop )
        stop = _hiltiRunHook(state()->unit, _hookForItem(state()->unit, field, true, false), field, true, value);

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, stop, done->block(), loop->block());
    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(done);

    _newValueForField(l->sharedPtr<Production>(), field, nullptr);
    _finishedProduction(l->sharedPtr<Production>());

    // Leave builder on stack.
}

void ParserBuilder::visit(type::Address* a)
{
    auto field = arg1();

    auto v4 = field->attributes()->has("ipv4");
    auto v6 = field->attributes()->has("ipv6");
    assert((v4 || v6) && ! (v4 && v6));

    auto byteorder = _fieldByteOrder(field, state()->unit);
    auto hltbo = byteorder ? cg()->hiltiExpression(byteorder) : hilti::builder::id::create("BinPAC::ByteOrder::Big");

    string big, little, host;

    if ( v4 ) {
        host = "IPv4";
        little= "IPv4Little";
        big = "IPv4Big";
        // FIXME: We don't have HILTI enums for little endian addresses.
    }
    else {
        host = "IPv6";
        little= "IPv6Little";
        big = "IPv6Big";
    }

    auto t1 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Little"),
        hilti::builder::id::create(string("Hilti::Packed::") + little) });

    auto t2 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Big"),
        hilti::builder::id::create(string("Hilti::Packed::") + big) });

    auto t3 = hilti::builder::tuple::create({
        hilti::builder::id::create("BinPAC::ByteOrder::Host"),
        hilti::builder::id::create(string("Hilti::Packed::") + host) });

    auto tuple = hilti::builder::tuple::create({ t1, t2, t3 });
    auto fmt = cg()->moduleBuilder()->addTmp("fmt", hilti::builder::type::byName("Hilti::Packed"));
    cg()->builder()->addInstruction(fmt, hilti::instruction::Misc::SelectValue, hltbo, tuple);

    auto end = cg()->builder()->addTmp("end", _hiltiTypeIteratorBytes(), nullptr, true);
    cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);

    auto iters = hilti::builder::tuple::create({ state()->cur, end });
    auto result = hiltiUnpack(a->sharedPtr<type::Address>(), iters, fmt);
    setResult(result);
}

void ParserBuilder::visit(type::Bitset* b)
{
}

void ParserBuilder::visit(type::Bool* b)
{
}

void ParserBuilder::_hiltiCheckChunk(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> length_op)
{
    auto chunked = field->attributes()->lookup("chunked");

    if ( ! chunked )
        return;

    auto end = cg()->moduleBuilder()->addTmp("eoc", _hiltiTypeIteratorBytes());
    cg()->builder()->addInstruction(end, hilti::instruction::operator_::End, state()->data);

    auto chunk_len = cg()->moduleBuilder()->addTmp("chunk_len", hilti::builder::integer::type(64));
    cg()->builder()->addInstruction(chunk_len, hilti::instruction::bytes::Diff, state()->cur, end);

    assert(chunked->value());
    auto target_size = cg()->hiltiExpression(chunked->value());

    auto large_enough = cg()->moduleBuilder()->addTmp("large_enough", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(large_enough, hilti::instruction::integer::Ugeq, chunk_len, target_size);

    auto blocks = cg()->builder()->addIf(large_enough);
    auto use_chunk = std::get<0>(blocks);
    auto cont = std::get<1>(blocks);

    cg()->moduleBuilder()->pushBuilder(use_chunk);
    // Chunk has the required size, pass it on.
    auto chunk = cg()->moduleBuilder()->addTmp("chunk", _hiltiTypeBytes());
    cg()->builder()->addInstruction(chunk, hilti::instruction::bytes::Sub, state()->cur, end);

    _newValueForField(nullptr, field, chunk);

    if ( field->attributes()->has("length") ) {
        assert(length_op);
        cg()->builder()->addInstruction(length_op, hilti::instruction::integer::Sub, length_op, chunk_len);
    }

    // Move beyond the chunk.

    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, end);
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::Assign, end);
    cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, end);
    cg()->builder()->addInstruction(hilti::instruction::operator_::Clear, chunk);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(use_chunk);

    cg()->moduleBuilder()->pushBuilder(cont); // Leave on stack.
}

void ParserBuilder::visit(type::Bytes* b)
{
    auto field = arg1();
    auto end = cg()->builder()->addTmp("end", _hiltiTypeIteratorBytes(), nullptr, true);
    cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);

    auto iters = hilti::builder::tuple::create({ state()->cur, end });

    auto len = field->attributes()->lookup("length");
    auto eod = field->attributes()->lookup("eod");
    auto until = field->attributes()->lookup("until");

    if ( len ) {
        auto length = cg()->hiltiExpression(len->value(), std::make_shared<type::Integer>(64, false));
        auto length_op = cg()->builder()->addTmp("unpack_len", hilti::builder::integer::type(64)); // This will be adapted by _hiltiCheckChunk().
        cg()->builder()->addInstruction(length_op, hilti::instruction::operator_::Assign, length);

        auto op1 = iters;
        auto op2 = hilti::builder::id::create("Hilti::Packed::BytesFixed");
        auto result_val = hiltiUnpack(b->sharedPtr<type::Bytes>(), op1, op2, length_op, [&] () { _hiltiCheckChunk(field, length_op); });
        setResult(result_val);
        return;
    }

    if ( until ) {
        auto op1 = iters;
        auto op2 = hilti::builder::id::create("Hilti::Packed::BytesDelim");
        auto op3 = cg()->hiltiExpression(until->value());
        auto result_val = hiltiUnpack(b->sharedPtr<type::Bytes>(), op1, op2, op3, [&] () { _hiltiCheckChunk(field, nullptr); });
        setResult(result_val);
        return;
    }

    if ( eod ) {
        auto loop = cg()->moduleBuilder()->newBuilder("eod-loop");
        auto done = cg()->moduleBuilder()->newBuilder("eod-reached");
        auto suspend = cg()->moduleBuilder()->newBuilder("eod-suspend");

        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

        cg()->moduleBuilder()->pushBuilder(loop);
        auto eod = cg()->builder()->addTmp("eod", hilti::builder::boolean::type());
        cg()->builder()->addInstruction(eod, hilti::instruction::bytes::IsFrozenIterBytes, state()->cur);
        cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, eod, done->block(), suspend->block());
        cg()->moduleBuilder()->popBuilder(loop);

        cg()->moduleBuilder()->pushBuilder(suspend);
        _hiltiCheckChunk(field, nullptr);
        _hiltiInsufficientInputHandler(true);
        cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data); // TODO: Nop?
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());
        cg()->moduleBuilder()->popBuilder(suspend);

        cg()->moduleBuilder()->pushBuilder(done);

        auto result_val = cg()->builder()->addTmp("unpacked_val", _hiltiTypeBytes());
        cg()->builder()->addInstruction(result_val, hilti::instruction::bytes::Sub, state()->cur, end);
        cg()->builder()->addInstruction(state()->cur, hilti::instruction::iterBytes::End, state()->data);

        setResult(result_val);
        return;
    }
}

void ParserBuilder::visit(type::Double* d)
{
}

void ParserBuilder::visit(type::Enum* e)
{
}

void ParserBuilder::visit(type::Integer* i)
{
    auto field = arg1();

    auto end = cg()->builder()->addTmp("end", _hiltiTypeIteratorBytes(), nullptr, true);
    cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);

    auto iters = hilti::builder::tuple::create({ state()->cur, end });
    auto byteorder = _fieldByteOrder(field, state()->unit);
    auto fmt = _hiltiIntUnpackFormat(i->width(), i->signed_(), byteorder);

    auto result = hiltiUnpack(i->sharedPtr<type::Integer>(), iters, fmt);
    setResult(result);
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

void ParserBuilder::visit(type::Sink* s)
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

void ParserBuilder::visit(type::unit::item::field::Ctor* c)
{
}

void ParserBuilder::visit(type::unit::item::field::Switch* s)
{
}

void ParserBuilder::visit(type::unit::item::field::AtomicType* t)
{
}

void ParserBuilder::visit(type::unit::item::field::Unit* t)
{
}

void ParserBuilder::visit(type::unit::item::field::switch_::Case* c)
{
}

void ParserBuilder::visit(type::Vector* v)
{
}

