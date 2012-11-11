
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

static shared_ptr<hilti::Type> _hiltiTypeParseResult()
{
    // (__cur, __lahead, __lahstart).
    hilti::builder::type_list ttypes = { _hiltiTypeIteratorBytes(), _hiltiTypeLookAhead(), _hiltiTypeIteratorBytes() };
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
    typedef std::list<shared_ptr<hilti::Expression>> parameter_list;

    ParserState(shared_ptr<binpac::type::Unit> unit,
           shared_ptr<hilti::Expression> self = nullptr,
           shared_ptr<hilti::Expression> data = nullptr,
           shared_ptr<hilti::Expression> cur = nullptr,
           shared_ptr<hilti::Expression> lahead = nullptr,
           shared_ptr<hilti::Expression> lahstart = nullptr,
           shared_ptr<hilti::Expression> cookie = nullptr
           );

    shared_ptr<hilti::Expression> hiltiArguments() const;

    shared_ptr<binpac::type::Unit> unit;
    shared_ptr<hilti::Expression> self;
    shared_ptr<hilti::Expression> data;
    shared_ptr<hilti::Expression> cur;
    shared_ptr<hilti::Expression> lahead;
    shared_ptr<hilti::Expression> lahstart;
    shared_ptr<hilti::Expression> cookie;

    // The mode onveys to the parsers of literals what the caller wants them
    // to do. This is needed to for doing look-ahead parsing, and hence not
    // relevant for fields that aren't literals.
    enum LiteralMode {
        // Normal parsing: parse field and raise parse error if not possible.
        DEFAULT,

        // Try to parse the field, but do not raise an error if it fails. If
        // it works, move cur as normal; if it fails, leave cur untouched.
        TRY,

        // We have parsed the field before as part of a look-ahead, and know
        // that it matches. Now we need to get actualy value for the iterator
        // range (lahstart, cur).
        LAHEAD_REPARSE,
    } mode;
};

ParserState::ParserState(shared_ptr<binpac::type::Unit> arg_unit,
               shared_ptr<hilti::Expression> arg_self,
               shared_ptr<hilti::Expression> arg_data,
               shared_ptr<hilti::Expression> arg_cur,
               shared_ptr<hilti::Expression> arg_lahead,
               shared_ptr<hilti::Expression> arg_lahstart,
               shared_ptr<hilti::Expression> arg_cookie
               )
{
    unit = arg_unit;
    self = (arg_self ? arg_self : hilti::builder::id::create("__self"));
    data = (arg_data ? arg_data : hilti::builder::id::create("__data"));
    cur = (arg_cur ? arg_cur : hilti::builder::id::create("__cur"));
    lahead = (arg_lahead ? arg_lahead : hilti::builder::id::create("__lahead"));
    lahstart = (arg_lahstart ? arg_lahstart : hilti::builder::id::create("__lahstart"));
    cookie = (arg_cookie ? arg_cookie : hilti::builder::id::create("__cookie"));
    mode = DEFAULT;
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
        auto true_ = std::get<0>(blocks);
        cont = std::get<1>(blocks);
        cg()->moduleBuilder()->pushBuilder(true_);
    }

    if ( field && field->attributes()->has("parse") ) {
        // Change input to what &parse attribute gives.
        auto input = cg()->hiltiExpression(field->attributes()->lookup("parse")->value());

        auto cur = cg()->builder()->addTmp("parse_cur", _hiltiTypeIteratorBytes());
        auto lah = cg()->builder()->addTmp("parse_lahead", _hiltiTypeLookAhead(), _hiltiLookAheadNone());
        auto lahstart = cg()->builder()->addTmp("parse_lahstart", _hiltiTypeIteratorBytes());
        cg()->builder()->addInstruction(cur, hilti::instruction::iterBytes::Begin, input);

        pstate = std::make_shared<ParserState>(state()->unit, state()->self, input, cur, lah, lahstart, state()->cookie);
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
        _hiltiDefineHook(_hookForUnit(unit, id->local()), false, unit, hook->body(), nullptr, hook->priority());

    else {
        auto i = unit->item(id);
        assert(i && i->type());
        auto dd = hook->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;
        _hiltiDefineHook(_hookForItem(unit, i, hook->foreach(), false), hook->foreach(), unit, hook->body(), dd, hook->priority());
    }
}

void ParserBuilder::hiltiRunFieldHooks(shared_ptr<type::unit::Item> item)
{
    _hiltiRunHook(_hookForItem(state()->unit, item, false, true), false, nullptr);
    _hiltiRunHook(_hookForItem(state()->unit, item, false, false), false, nullptr);
}

void ParserBuilder::hiltiUnitHooks(shared_ptr<type::Unit> unit)
{
    for ( auto i : unit->flattenedItems() ) {
        if ( ! i->type() )
            continue;

        for ( auto h : i->hooks() ) {
            auto dd = h->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;
            _hiltiDefineHook(_hookForItem(unit, i->sharedPtr<type::unit::Item>(), h->foreach(), true),
                             h->foreach(), unit, h->body(), dd, h->priority());
        }
    }

    for ( auto g : unit->globalHooks() ) {
        for ( auto h : g->hooks() ) {

            if ( util::startsWith(g->id()->local(), "%") )
                _hiltiDefineHook(_hookForUnit(unit, g->id()->local()), false, unit, h->body(), nullptr, h->priority());

            else {
                auto i = unit->item(g->id());
                assert(i && i->type());
                auto dd = h->foreach() ? ast::type::checkedTrait<type::trait::Container>(i->type())->elementType() : nullptr;
                _hiltiDefineHook(_hookForItem(unit, i, h->foreach(), false), h->foreach(), unit, h->body(), dd, h->priority());
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

    hilti::builder::list::element_list mtypes;

    for ( auto p : unit->properties() ) {
        if ( p->id()->name() != "%mimetype" )
            continue;

        mtypes.push_back(cg()->hiltiExpression(p->value()));
        }


    auto mime_types = hilti::builder::list::create(hilti::builder::string::type(), mtypes, unit->location());

    cg()->builder()->addInstruction(parser, hilti::instruction::struct_::New,
                                    hilti::builder::id::create("BinPACHilti::Parser"));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("name"),
                                    hilti::builder::string::create(unit->id()->name()));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("description"),
                                    hilti::builder::string::create("No description yet."));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("params"),
                                    hilti::builder::integer::create(unit->parameters().size()));

    cg()->builder()->addInstruction(hilti::instruction::struct_::Set,
                                    parser,
                                    hilti::builder::string::create("mime_types"),
                                    mime_types);

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

    hilti_expression_list uparams;

    for ( auto p : state()->unit->parameters() )
        uparams.push_back(hilti::builder::id::create(p->id()->name()));

    _prepareParseObject(uparams, hilti::builder::id::create("__sink"), hilti::builder::id::create("__mimetype"));

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
        hilti_expression_list params;

        for ( auto p : state()->unit->parameters() )
            params.push_back(hilti::builder::id::create(p->id()->name()));

        _prepareParseObject(params);
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

    _finalizeParseObject();

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
    auto func = _newParseFunction(name, unit);

    if ( unit->exported() )
        _hiltiFilterInput(false);

    _last_parsed_value = nullptr;
    _store_values = 1;
    bool success = parse(grammar->root());
    assert(success);

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
    auto arg6 = hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), false, nullptr);

    auto rtype = hilti::builder::function::result(_hiltiTypeParseResult());

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

        auto ftype = itemType(f);
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
            def = cg()->hiltiDefault(v->type());

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

    return hilti::builder::struct_::type(fields, u->location());
}

shared_ptr<hilti::Expression> ParserBuilder::_allocateParseObject(shared_ptr<Type> unit, bool store_in_self)
{
    auto rt = cg()->hiltiType(unit);
    auto ut = ast::checkedCast<hilti::type::Reference>(rt)->argType();

    auto pobj = store_in_self ? state()->self : cg()->builder()->addTmp("pobj", rt);

    cg()->builder()->addInstruction(pobj, hilti::instruction::struct_::New, hilti::builder::type::create(ut));

    return pobj;
}

void ParserBuilder::_prepareParseObject(const hilti_expression_list& params, shared_ptr<hilti::Expression> sink, shared_ptr<hilti::Expression> mimetype)
{
    // Initialize the parameter fields.
    auto arg = params.begin();
    auto u = state()->unit;

    for ( auto p : u->parameters() ) {
        assert(arg != params.end());
        auto field = hilti::builder::string::create(util::fmt("__p_%s", p->id()->name()));
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, field, *arg++);
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

    // Initialize variables with either &defaults where given, or their HILTI defaults (and sinks explicitly).
    for ( auto v : u->variables() ) {
        auto id = hilti::builder::string::create(v->id()->name());
        auto attr = v->attributes()->lookup("default");
        auto def = attr ? cg()->hiltiExpression(attr->value()) : cg()->hiltiDefault(v->type());

        if ( def )
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, id, def);

        if ( ast::isA<type::Sink>(v->type()) ) {
            sink = cg()->builder()->addTmp("sink", cg()->hiltiType(v->type()));
            cg()->builder()->addInstruction(sink, hilti::instruction::flow::CallResult,
                                            hilti::builder::id::create("BinPACHilti::sink_new"),
                                            hilti::builder::tuple::create({}));
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, id, sink);
        }
    }

    if ( u->buffering() )
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__input"), state()->cur);

    if ( u->exported() ) {
        cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__parser"), _hiltiParserDefinition(u));

        if ( mimetype )
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__mimetype"), mimetype);

        if ( sink )
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self, hilti::builder::string::create("__sink"), sink);
    }

    _hiltiRunHook(_hookForUnit(state()->unit, "%init"), false, nullptr);
}

void ParserBuilder::_finalizeParseObject()
{
    auto unit = state()->unit;

    _hiltiRunHook(_hookForUnit(unit, "%done"), false, nullptr);

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
}

void ParserBuilder::_startingProduction(shared_ptr<Production> p, shared_ptr<type::unit::item::Field> field)
{
    cg()->builder()->addComment(util::fmt("Production: %s", util::strtrim(p->render().c_str())));
    cg()->builder()->addComment("");

    if ( cg()->debugLevel() ) {
        _hiltiDebugVerbose(util::fmt("parsing %s", util::strtrim(p->render().c_str())));
        _hiltiDebugShowToken("look-ahead", state()->lahead);
        _hiltiDebugShowInput("input", state()->cur);
    }

    if ( ! field || ! storingValues() )
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
    auto ftype = itemType(field);
    auto def = cg()->hiltiDefault(ftype);

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

void ParserBuilder::_newValueForField(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> value)
{
    auto name = field->id()->name();

    if ( value )
        value = _hiltiProcessFieldValue(field, value);

    if ( cg()->debugLevel() > 0 && ! ast::isA<type::Unit>(field->type()))
        cg()->builder()->addDebugMsg("binpac", util::fmt("%s = %%s", name), value);

    if ( value ) {
        if ( storingValues() ) {
            cg()->builder()->addInstruction(hilti::instruction::struct_::Set, state()->self,
                                            hilti::builder::string::create(name), value);
        }
    }

    else {
        auto ftype = itemType(field);
        value = cg()->moduleBuilder()->addTmp("field-val", cg()->hiltiType(ftype));
        cg()->builder()->addInstruction(value, hilti::instruction::struct_::Get, state()->self,
                                        hilti::builder::string::create(name));
    }

    _hiltiSaveInputPostion();

    hiltiRunFieldHooks(field);

    _hiltiUpdateInputPostion();

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

    cg()->builder()->addDebugMsg("binpac-verbose", "- %s is %s", hilti::builder::string::create(tag), token);
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

    cg()->builder()->addDebugMsg("binpac-verbose", "- %s is |%s...| (lit-mode: %s; frozen: %s)",
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

void ParserBuilder::_hiltiDefineHook(shared_ptr<ID> id, bool foreach, shared_ptr<type::Unit> unit, shared_ptr<Statement> body, shared_ptr<Type> dollardollar, int priority)
{
    auto t = _hookName(id->pathAsString());
    auto local = t.first;
    auto name = t.second;

    hilti::builder::function::parameter_list p = {
        hilti::builder::function::parameter("__self", cg()->hiltiTypeParseObjectRef(unit), false, nullptr),
        hilti::builder::function::parameter("__cookie", cg()->hiltiTypeCookie(), false, nullptr)
    };

    if ( dollardollar ) {
        p.push_back(hilti::builder::function::parameter("__dollardollar", cg()->hiltiType(dollardollar), false, nullptr));
        cg()->hiltiBindDollarDollar(hilti::builder::id::create("__dollardollar"));
    }

    shared_ptr<hilti::Type> rtype = nullptr;

    if ( foreach )
        rtype = hilti::builder::boolean::type();
    else
        rtype = hilti::builder::void_::type();

    cg()->moduleBuilder()->pushHook(name, hilti::builder::function::result(rtype), p, nullptr, priority, 0, false);

    pushState(std::make_shared<ParserState>(unit));

    auto msg = util::fmt("- executing hook %s@%s", id->pathAsString(), string(body->location()));
    _hiltiDebugVerbose(msg);

    cg()->hiltiStatement(body);

    if ( dollardollar )
        cg()->hiltiUnbindDollarDollar();

    popState();

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

shared_ptr<hilti::Expression> ParserBuilder::_hiltiRunHook(shared_ptr<ID> id, bool foreach, shared_ptr<hilti::Expression> dollardollar)
{
    auto msg = util::fmt("- triggering hook %s", id->pathAsString());
    _hiltiDebugVerbose(msg);

    auto t = _hookName(id->pathAsString());
    auto local = t.first;
    auto name = t.second;

    // Declare the hook if it's in our module and we don't have done that yet.
    if ( local && ! cg()->moduleBuilder()->lookupNode("hook", name) ) {
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

shared_ptr<hilti::Expression> ParserBuilder::_hiltiMatchTokenInit(const string& name, const std::list<shared_ptr<production::Literal>>& literals)
{
    auto mstate = cg()->builder()->addTmp("match", _hiltiTypeMatchTokenState());

    auto glob = cg()->moduleBuilder()->lookupNode("match-token", name);

    if ( ! glob ) {
        std::list<std::pair<string, string>> tokens;

        for ( auto l : literals ) {
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
                                                                             std::list<shared_ptr<production::Literal>> expected
                                                                            )
{
    // Not found.
    auto b = cg()->moduleBuilder()->cacheBlockBuilder("not-found", [&] () {
        _hiltiParseError("expected symbol(s) not found");
    });

    cases->push_back(std::make_pair(_hiltiLookAheadNone(), b));

    // Insufficient input.
    b = cg()->moduleBuilder()->cacheBlockBuilder("insufficient-input", [&] () {
        _hiltiYieldAndTryAgain(prod, b, repeat);
    });

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

void ParserBuilder::_hiltiYieldAndTryAgain(shared_ptr<Production> prod, shared_ptr<hilti::builder::BlockBuilder> builder, shared_ptr<hilti::builder::BlockBuilder> cont)
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

        auto at_eod = _hiltiInsufficientInputHandler();
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

    cg()->builder()->pushCatchAll();
    cg()->builder()->addThrow("BinPACHilti::ParseError", hilti::builder::string::create("unpack failed"));
    cg()->builder()->popCatchAll();

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

    // See if the unit has a byte order property defined.
    auto prop_order = unit->property("byteorder");

    if ( prop_order )
        order = prop_order->value();

    // See if the field has a byte order defined.
    auto field_order = field->attributes()->lookup("byteorder");

    if ( field_order )
        order = field_order->value();

    else {
        // See if the unit has a byteorder property.
        auto unit_order = unit->property("byteorder");

        if ( unit_order )
            order = unit_order->value();
    }

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

shared_ptr<binpac::Type> ParserBuilder::itemType(shared_ptr<type::unit::Item> item)
{
    auto field = ast::tryCast<type::unit::item::Field>(item);

    if ( ! field )
        return item->type();

    auto ftype = ast::type::checkedTrait<type::trait::Parseable>(field->type())->fieldType();

    auto convert = field->attributes()->lookup("convert");

    if ( convert )
        return convert->value()->type();

    return ftype;
}

shared_ptr<hilti::Expression> ParserBuilder::_hiltiProcessFieldValue(shared_ptr<type::unit::item::Field> field, shared_ptr<hilti::Expression> value)
{
    auto ftype = ast::type::checkedTrait<type::trait::Parseable>(field->type())->fieldType();
    auto convert = field->attributes()->lookup("convert");

    if ( convert ) {
        cg()->hiltiBindDollarDollar(value);
        auto dd = cg()->builder()->addTmp("__dollardollar", value->type(), value);
        value = cg()->hiltiExpression(convert->value());
        cg()->hiltiUnbindDollarDollar();
    }

    return value;
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

    // We parse the field normally according to its type, and then compare
    // the value against the constant we expec.t
    shared_ptr<hilti::Expression> value;
    processOne(field->type(), &value, field);

    if ( state()->mode != ParserState::LAHEAD_REPARSE ) {

        auto expr = std::make_shared<expression::Constant>(i->sharedPtr<constant::Integer>());
        auto mismatch = cg()->builder()->addTmp("mismatch", hilti::builder::boolean::type());
        cg()->builder()->addInstruction(mismatch, hilti::instruction::operator_::Unequal, value, cg()->hiltiExpression(expr));

        auto branches = cg()->builder()->addIf(mismatch);
        auto error = std::get<0>(branches);
        auto cont = std::get<1>(branches);

        cg()->moduleBuilder()->pushBuilder(error);
        _hiltiParseError(util::fmt("integer constant expected (%s)", i->render()));
        cg()->moduleBuilder()->popBuilder(error);

        cg()->moduleBuilder()->pushBuilder(cont);
    }

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

    cg()->builder()->addComment(util::fmt("RegExpt ctor: %s", field->id()->name()));

    auto done = cg()->moduleBuilder()->newBuilder("done-regexp");
    auto loop = cg()->moduleBuilder()->newBuilder("regexp-loop");

    auto token_id = hilti::builder::integer::create(lit->tokenID());
    auto name = util::fmt("__literal_%d", lit->tokenID());

    auto symbol = cg()->moduleBuilder()->addTmp("token", hilti::builder::integer::type(32), nullptr, true);
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
                                    found_lit->addInstruction(hilti::instruction::flow::Jump, done->block());
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

    _startingProduction(c->sharedPtr<Production>(), field);

    auto child = c->childType();
    auto subself = _allocateParseObject(child, false);

    auto pstate = std::make_shared<ParserState>(child, subself, state()->data, state()->cur, state()->lahead, state()->lahstart, state()->cookie);
    pushState(pstate);

    hilti_expression_list params;

    for ( auto p : field->parameters() )
        params.push_back(cg()->hiltiExpression(p));

    _prepareParseObject(params);

    auto child_result = cg()->builder()->addTmp("presult", _hiltiTypeParseResult());
    auto child_func = cg()->hiltiParseFunction(child);

    if ( cg()->debugLevel() > 0 ) {
        _hiltiDebug(field->id()->name());
        cg()->builder()->debugPushIndent();
    }

    cg()->builder()->addInstruction(child_result, hilti::instruction::flow::CallResult, child_func, state()->hiltiArguments());

    if ( cg()->debugLevel() > 0 )
        cg()->builder()->debugPopIndent();

    _finalizeParseObject();

    popState();

    cg()->builder()->addInstruction(state()->cur, hilti::instruction::tuple::Index, child_result, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(state()->lahead, hilti::instruction::tuple::Index, child_result, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(state()->lahstart, hilti::instruction::tuple::Index, child_result, hilti::builder::integer::create(2));

    _newValueForField(field, subself);
    _finishedProduction(c->sharedPtr<Production>());
}

void ParserBuilder::visit(production::Counter* c)
{
    auto field = c->pgMeta()->field;
    assert(field);

    _startingProduction(c->sharedPtr<Production>(), field);

    auto i = cg()->builder()->addTmp("count-i", hilti::builder::integer::type(64), cg()->hiltiExpression(c->expression()));
    auto b = cg()->builder()->addTmp("count-bool", hilti::builder::boolean::type());
    auto cont = cg()->moduleBuilder()->newBuilder("count-done");
    auto parse_one = cg()->moduleBuilder()->newBuilder("count-parse-one");

    auto loop = cg()->moduleBuilder()->pushBuilder("count-loop");
    cg()->builder()->addInstruction(b, hilti::instruction::integer::Sleq, i, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, b, cont->block(), parse_one->block());
    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(parse_one);

    disableStoringValues();
    parse(c->body());
    enableStoringValues();

    // Run foreach hook.
    assert(_last_parsed_value);
    auto stop = _hiltiRunHook(_hookForItem(state()->unit, field, true, false), true, _last_parsed_value);
    cg()->builder()->addInstruction(i, hilti::instruction::integer::Decr, i);
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, stop, cont->block(), loop->block());

    cg()->moduleBuilder()->popBuilder(parse_one);

    cg()->moduleBuilder()->pushBuilder(cont);

    _newValueForField(field, nullptr);
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

    cg()->builder()->addComment(util::fmt("Literal: %s", field->id()->name()));

    auto literal = cg()->moduleBuilder()->addTmp("literal", cg()->hiltiType(l->type()));

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

    _newValueForField(field, literal);
    _finishedProduction(l->sharedPtr<Production>());

    // Leave builder on stack.
}

void ParserBuilder::visit(production::LookAhead* l)
{
    _startingProduction(l->sharedPtr<Production>(), nullptr);

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
            auto expr = cg()->hiltiExpression(e);
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

    auto expr = cg()->hiltiExpression(s->expression());
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

    _newValueForField(field, value);
    _finishedProduction(v->sharedPtr<Production>());
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
    parse(w->body());
    enableStoringValues();

    auto cond = cg()->hiltiExpression(w->expression(), std::make_shared<type::Bool>());

    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, cond, done->block(), cont->block());
    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(cont);
    assert(_last_parsed_value);
    auto stop = _hiltiRunHook(_hookForItem(state()->unit, field, true, false), true, _last_parsed_value);
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, stop, done->block(), loop->block());
    cg()->moduleBuilder()->popBuilder(cont);

    // Run foreach hook.

    cg()->moduleBuilder()->pushBuilder(done);

    _newValueForField(field, nullptr);
    _finishedProduction(w->sharedPtr<Production>());

    // Leave builder on stack.
}

#endif

void ParserBuilder::visit(production::Loop* l)
{
    auto field = l->pgMeta()->field;
    assert(field);

    _startingProduction(l->sharedPtr<Production>(), field);

    auto done = cg()->moduleBuilder()->newBuilder("until-done");
    auto loop = cg()->moduleBuilder()->pushBuilder("until-loop");

    disableStoringValues();
    parse(l->body());
    enableStoringValues();

    // Run foreach hook.
    assert(_last_parsed_value);
    auto stop = _hiltiRunHook(_hookForItem(state()->unit, field, true, false), true, _last_parsed_value);
    cg()->builder()->addInstruction(hilti::instruction::flow::IfElse, stop, done->block(), loop->block());
    cg()->moduleBuilder()->popBuilder(loop);

    cg()->moduleBuilder()->pushBuilder(done);

    _newValueForField(field, nullptr);
    _finishedProduction(l->sharedPtr<Production>());

    // Leave builder on stack.
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
    auto end = cg()->builder()->addTmp("end", _hiltiTypeIteratorBytes(), nullptr, true);
    cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);

    auto iters = hilti::builder::tuple::create({ state()->cur, end });

    auto len = field->attributes()->lookup("length");
    auto eod = field->attributes()->lookup("eod");

    if ( len ) {
        auto op1 = iters;
        auto op2 = hilti::builder::id::create("Hilti::Packed::BytesFixed");
        auto op3 = cg()->hiltiExpression(len->value());
        auto result_val = hiltiUnpack(b->sharedPtr<type::Bytes>(), op1, op2, op3);
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
        // have_chunk()
        _hiltiInsufficientInputHandler(true);
        cg()->builder()->addInstruction(end, hilti::instruction::iterBytes::End, state()->data);
        cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());
        cg()->moduleBuilder()->popBuilder(suspend);

        cg()->moduleBuilder()->pushBuilder(done);

        auto result_val = cg()->builder()->addTmp("unpacked_val", _hiltiTypeBytes());
        cg()->builder()->addInstruction(result_val, hilti::instruction::bytes::Sub, state()->cur, end);

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

