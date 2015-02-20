
#include <hilti/hilti.h>

#include "synchronizer.h"
#include "../production.h"
#include "../type.h"
#include "../attribute.h"
#include "../expression.h"
#include "../options.h"
#include "../grammar.h"

using namespace binpac;
using namespace binpac::codegen;

void Synchronizer::_hiltiNotYetFound(shared_ptr<hilti::builder::BlockBuilder> cont, const std::string& tag)
{
    auto eod = cg()->builder()->addTmp("eod", hilti::builder::boolean::type());
    cg()->builder()->addInstruction(eod, hilti::instruction::bytes::IsFrozenIterBytes, state()->cur);

    auto branches = cg()->builder()->addIfElse(eod);
    auto error = std::get<0>(branches);
    auto suspend = std::get<1>(branches);

    // Not found and end of input reached.
    cg()->moduleBuilder()->pushBuilder(error);
    _hiltiSynchronizeError("end of input reached without finding " + tag);
    cg()->moduleBuilder()->popBuilder(error);

    // Not found but more input may come, suspend.
    cg()->moduleBuilder()->pushBuilder(suspend);
    _hiltiDebugVerbose("out of input, yielding ...");
    cg()->builder()->addInstruction(hilti::instruction::flow::YieldUntil, state()->data);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, cont->block());
    cg()->moduleBuilder()->popBuilder(suspend);
}

void Synchronizer::_hiltiSynchronizeError(const string& msg)
{
    _hiltiDebugVerbose("triggering parse error");
    // TODO: Should this raise a separate SynchronizationError instead?
    auto etype = builder::type::byName("BinPACHilti::ParseError");
    auto excpt = cg()->builder()->addTmp("excpt", hilti::builder::reference::type(etype));
    auto arg = hilti::builder::string::create("synchronization error: " + msg);

    cg()->builder()->addInstruction(excpt, ::hilti::instruction::exception::NewWithArg,
                                    hilti::builder::type::create(etype),
                                    arg);

    cg()->builder()->addInstruction(hilti::instruction::exception::Throw, excpt);
}

void Synchronizer::_hiltiSynchronizeOnBytes(const string& data)
{
    auto mtype = hilti::builder::tuple::type({ hilti::builder::boolean::type(), hilti::builder::iterator::typeBytes() });

    auto match = cg()->builder()->addTmp("m", mtype);
    auto success = cg()->builder()->addTmp("success", hilti::builder::boolean::type());

    auto loop = cg()->moduleBuilder()->newBuilder("loop");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    cg()->moduleBuilder()->pushBuilder(loop);

    cg()->builder()->addInstruction(match, hilti::instruction::bytes::FindAtIter, state()->cur, hilti::builder::bytes::create(data));
    cg()->builder()->addInstruction(success, hilti::instruction::tuple::Index, match, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::tuple::Index, match, hilti::builder::integer::create(1));

    auto branches = cg()->builder()->addIfElse(success);
    auto found = std::get<0>(branches);
    auto not_found = std::get<1>(branches);
    auto done = std::get<2>(branches);

    cg()->moduleBuilder()->popBuilder(loop);

    // Found the pattern.

    cg()->moduleBuilder()->pushBuilder(found);

    if ( state()->type == SynchronizeAfter )
        // Skip pattern.
        cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::IncrBy, state()->cur, hilti::builder::integer::create(data.size()));

    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());

    cg()->moduleBuilder()->popBuilder(found);

    // Haven't found the pattern (yet).

    cg()->moduleBuilder()->pushBuilder(not_found);

    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    _hiltiNotYetFound(loop, "bytes");

    cg()->moduleBuilder()->popBuilder(not_found);

    // Continue after finding pattern.

    cg()->moduleBuilder()->pushBuilder(done);
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

void Synchronizer::_hiltiSynchronizeOnRegexp(const hilti::builder::regexp::re_pattern_list& patterns)
{
    auto mtype = hilti::builder::tuple::type({ hilti::builder::boolean::type(), hilti::builder::iterator::typeBytes() });

    auto mstate = cg()->builder()->addTmp("match", _hiltiTypeMatchTokenState());
    auto mresult = cg()->builder()->addTmp("match_result", _hiltiTypeMatchResult());
    auto eob = cg()->builder()->addTmp("eob", hilti::builder::iterator::typeBytes());
    auto rc = cg()->builder()->addTmp("rc", hilti::builder::integer::type(32));
    auto b = cg()->builder()->addTmp("b", hilti::builder::boolean::type());
    auto i = cg()->builder()->addTmp("i", hilti::builder::iterator::typeBytes());

    auto loop_inner = cg()->moduleBuilder()->newBuilder("loop_inner");
    auto loop_outer = cg()->moduleBuilder()->newBuilder("loop_outer");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop_outer->block());

    ////

    cg()->moduleBuilder()->pushBuilder(loop_outer);

    // match_token_init
    auto op = hilti::builder::regexp::create(patterns);
    auto re = ast::checkedCast<hilti::ctor::RegExp>(op->ctor());
    re->attributes().add(attribute::NOSUB);
    re->attributes().add(attribute::FIRSTMATCH);
    auto glob = cg()->moduleBuilder()->addGlobal(hilti::builder::id::node("__sync"), op->type(), op, hilti::AttributeSet(), true);
    auto pattern = ast::checkedCast<hilti::Expression>(glob);

    cg()->builder()->addInstruction(mstate, hilti::instruction::regexp::MatchTokenInit, pattern);
    cg()->builder()->addInstruction(i, hilti::instruction::operator_::Assign, state()->cur);

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop_inner->block());

    cg()->moduleBuilder()->popBuilder(loop_outer);

    ////

    cg()->moduleBuilder()->pushBuilder(loop_inner);

    // match_token_advance
    cg()->builder()->addInstruction(eob, hilti::instruction::iterBytes::End, state()->data);
    cg()->builder()->addInstruction(mresult, hilti::instruction::regexp::MatchTokenAdvanceBytes, mstate, i, eob);
    cg()->builder()->addInstruction(rc, hilti::instruction::tuple::Index, mresult, hilti::builder::integer::create(0));
    cg()->builder()->addInstruction(i, hilti::instruction::tuple::Index, mresult, hilti::builder::integer::create(1));
    cg()->builder()->addInstruction(b, hilti::instruction::integer::Slt, rc, hilti::builder::integer::create(0));

    auto branches = cg()->builder()->addIf(b);
    auto not_yet_found = std::get<0>(branches);
    auto cont = std::get<1>(branches);

    cg()->moduleBuilder()->popBuilder(loop_inner);

    cg()->moduleBuilder()->pushBuilder(cont);

    cg()->builder()->addInstruction(b, hilti::instruction::integer::Sgt, rc, hilti::builder::integer::create(0));
    auto branches2 = cg()->builder()->addIfElse(b);
    auto found = std::get<0>(branches2);
    auto not_found = std::get<1>(branches2);
    auto done = std::get<2>(branches2);

    cg()->moduleBuilder()->popBuilder(cont);

    ////

    // Not yet found the pattern, but more input could change that.
    cg()->moduleBuilder()->pushBuilder(not_yet_found);
    cg()->builder()->addInstruction(i, hilti::instruction::operator_::Assign, eob);
    _hiltiNotYetFound(loop_inner, "regular expression");
    cg()->moduleBuilder()->popBuilder(not_yet_found);

    // For sure not found at this position, try next.
    cg()->moduleBuilder()->pushBuilder(not_found);

    cg()->builder()->addInstruction(eob, hilti::instruction::iterBytes::End, state()->data);
    cg()->builder()->addInstruction(b, hilti::instruction::operator_::Equal, state()->cur, eob);
    auto branches3 = cg()->builder()->addIf(b);
    auto end_of_data = std::get<0>(branches3);
    auto increment = std::get<1>(branches3);

    cg()->moduleBuilder()->popBuilder(not_found);

    // Not found and end of input reached.
    cg()->moduleBuilder()->pushBuilder(end_of_data);
    _hiltiNotYetFound(not_found, "regular expression");
    cg()->moduleBuilder()->popBuilder(end_of_data);

    cg()->moduleBuilder()->pushBuilder(increment);
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::Incr, state()->cur);
    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop_outer->block());
    cg()->moduleBuilder()->popBuilder(increment);

    // Found the pattern.
    cg()->moduleBuilder()->pushBuilder(found);

    if ( state()->type == SynchronizeAfter )
        // Skip pattern.
        cg()->builder()->addInstruction(state()->cur, hilti::instruction::operator_::Assign, i);

    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());

    cg()->moduleBuilder()->popBuilder(found);

    // Continue after finding pattern.

    cg()->moduleBuilder()->pushBuilder(done);
}

Synchronizer::Synchronizer(CodeGen* cg)
    : CGVisitor<>(cg, "Synchronizer")
{
}

Synchronizer::~Synchronizer()
{
}

shared_ptr<hilti::Expression> Synchronizer::hiltiSynchronize(shared_ptr<Production> p, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> cur)
{
    assert(p->supportsSynchronize());

    _state.type = SynchronizeUnspecified;
    _state.data = data;
    _state.cur = cur;

    _hiltiSynchronizeOne(p);

    return _state.cur;
}

shared_ptr<hilti::Expression> Synchronizer::hiltiSynchronize(shared_ptr<type::Unit> unit, shared_ptr<hilti::Expression> data, shared_ptr<hilti::Expression> cur)
{
    assert(unit->supportsSynchronize());

    _state.type = SynchronizeUnspecified;
    _state.data = data;
    _state.cur = cur;

    _hiltiSynchronizeOne(unit);

    return _state.cur;
}

void Synchronizer::_hiltiSynchronizeOne(shared_ptr<Node> node)
{
    processOne(node);
}

SynchronizerState* Synchronizer::state()
{
    return &_state;
}

void Synchronizer::_hiltiDebug(const string& msg)
{
    if ( cg()->options().debug > 0 )
        cg()->builder()->addDebugMsg("binpac", msg);
}

void Synchronizer::_hiltiDebugVerbose(const string& msg)
{
    if ( cg()->options().debug > 0 )
        cg()->builder()->addDebugMsg("binpac-verbose", string("- ") + msg);
}

void Synchronizer::visit(ctor::Bytes* b)
{
    _hiltiSynchronizeOnBytes(b->value());
}

void Synchronizer::visit(ctor::RegExp* b)
{
    _hiltiSynchronizeOnRegexp(b->patterns());
}

void Synchronizer::visit(type::Unit* u)
{
    for ( auto p : u->properties() ) {
        auto attr = p->property();

        if ( attr->key() == "synchronize-after" ) {
            _state.type = SynchronizeAfter;
            _hiltiSynchronizeOne(attr->value());
        }

        if ( attr->key() == "synchronize-at" ) {
            _state.type = SynchronizeAt;
            _hiltiSynchronizeOne(attr->value());
        }
    }

    if ( u->grammar()->root()->supportsSynchronize() )
        _hiltiSynchronizeOne(u->grammar()->root());
}

void Synchronizer::visit(type::EmbeddedObject* o)
{
    auto type = o->argType();
    auto htype = cg()->hiltiType(type);

    auto at = cg()->builder()->addTmp("at_obj", ::hilti::builder::boolean::type());

    auto done = cg()->moduleBuilder()->newBuilder("sync-done");
    auto loop = cg()->moduleBuilder()->newBuilder("sync-loop");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    ///

    cg()->moduleBuilder()->pushBuilder(loop);

    cg()->builder()->addInstruction(state()->cur, hilti::instruction::bytes::NextObject, state()->cur);
    cg()->builder()->addInstruction(at, hilti::instruction::bytes::AtObject, state()->cur, ::hilti::builder::type::create(htype));

    auto branches = cg()->builder()->addIf(at);
    auto found = std::get<0>(branches);
    auto not_found = std::get<1>(branches);

    cg()->moduleBuilder()->popBuilder(loop);

    ///

    cg()->moduleBuilder()->pushBuilder(found);

    if ( state()->type == SynchronizeAfter )
        // Skip object.
        cg()->builder()->addInstruction(state()->cur, hilti::instruction::bytes::SkipObject, state()->cur);

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());

    cg()->moduleBuilder()->popBuilder(found);

    ///

    cg()->moduleBuilder()->pushBuilder(not_found);

    /// Check if there's *any* object here, and if so move past it.
    cg()->builder()->addInstruction(at, hilti::instruction::bytes::AtObject, state()->cur);

    auto branches2 = cg()->builder()->addIfElse(at);
    auto found2 = std::get<0>(branches2);
    auto not_found2 = std::get<1>(branches2);

    cg()->moduleBuilder()->popBuilder(not_found);

    cg()->moduleBuilder()->pushBuilder(found2);
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::bytes::SkipObject, state()->cur);
    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());
    cg()->moduleBuilder()->popBuilder(found2);

    cg()->moduleBuilder()->pushBuilder(not_found2);
    // Actually the end of the data.
    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    _hiltiNotYetFound(loop, "embedded object");
    cg()->moduleBuilder()->popBuilder(not_found2);

    /// Continue normally.

    cg()->moduleBuilder()->pushBuilder(done);
}

void Synchronizer::visit(type::Mark* m)
{
    auto done = cg()->moduleBuilder()->newBuilder("sync-done");
    auto loop = cg()->moduleBuilder()->newBuilder("sync-loop");

    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, loop->block());

    ///

    cg()->moduleBuilder()->pushBuilder(loop);

    auto at_mark = cg()->builder()->addTmp("at_mark", ::hilti::builder::boolean::type());
    cg()->builder()->addInstruction(state()->cur, hilti::instruction::bytes::NextMark, state()->cur);
    cg()->builder()->addInstruction(at_mark, hilti::instruction::bytes::AtMark, state()->cur);

    auto branches = cg()->builder()->addIf(at_mark);
    auto found = std::get<0>(branches);
    auto not_found = std::get<1>(branches);

    cg()->moduleBuilder()->popBuilder(loop);

    ///

    cg()->moduleBuilder()->pushBuilder(found);
    cg()->builder()->addInstruction(hilti::instruction::flow::Jump, done->block());
    cg()->moduleBuilder()->popBuilder(found);

    ///

    cg()->moduleBuilder()->pushBuilder(not_found);
    cg()->builder()->addInstruction(hilti::instruction::bytes::Trim, state()->data, state()->cur);
    _hiltiNotYetFound(loop, "mark");
    cg()->moduleBuilder()->popBuilder(not_found);

    /// Continue normally.

    cg()->moduleBuilder()->pushBuilder(done);
}

void Synchronizer::visit(expression::Ctor* b)
{
    _hiltiSynchronizeOne(b->ctor());
}

void Synchronizer::visit(expression::Type* t)
{
    _hiltiSynchronizeOne(t->typeValue());
}

void Synchronizer::visit(production::Literal* l)
{
    _state.type = SynchronizeAt;
    _hiltiSynchronizeOne(l->literal());
}

void Synchronizer::visit(production::Sequence* l)
{
    _hiltiSynchronizeOne(l->sequence().front());
}

void Synchronizer::visit(production::ChildGrammar* l)
{
    auto utype = ast::checkedCast<type::Unit>(l->type());
    _hiltiSynchronizeOne(utype);
}

void Synchronizer::visit(production::LookAhead* l)
{
    internalError("sync on look-ahead not yet supported");
}
