
#include "declaration.h"
#include "statement.h"
#include "variable.h"
#include "function.h"
#include "declaration.h"
#include "instruction.h"
#include "builder/nodes.h"
#include "passes/printer.h"
#include "hilti/autogen/instructions.h"

using namespace hilti;

uint64_t Statement::_counter = 0;

static void _addExpressionToVariables(Statement::variable_set* vars, shared_ptr<Expression> expr)
{
    if ( auto v = ast::tryCast<expression::Variable>(expr) ) {
        if ( auto l = ast::tryCast<variable::Local>(v->variable()) )
            vars->insert(std::make_shared<Statement::FlowVariable>(v));
    }

    if ( auto p = ast::tryCast<expression::Parameter>(expr) )
        vars->insert(std::make_shared<Statement::FlowVariable>(p));
}

Statement::FlowVariable::FlowVariable(shared_ptr<expression::Variable> var)
{
    auto v = var->variable();

    if ( auto local = ast::tryCast<variable::Local>(v) )
        name = local->internalName();
    else
        name = v->id()->name();

    id = v->id();
    expression = var;
}

Statement::FlowVariable::FlowVariable(shared_ptr<expression::Parameter> param)
{
    name = util::fmt("p:%s", param->parameter()->id()->name());
    id = param->parameter()->id();
    expression = param;
}

Statement::FlowVariable& Statement::FlowVariable::operator=(const FlowVariable& other)
{
    name = other.name;
    expression = other.expression;
    return *this;
}

bool Statement::FlowVariable::operator==(const FlowVariable& other) const
{
    return name == other.name;
}

bool Statement::FlowVariable::operator<(const FlowVariable& other) const
{
    return name < other.name;
}

Statement::Statement(const Location& l) : ast::Statement<AstInfo>(l)
{
    _number = ++_counter;
}

string Statement::render()
{
    std::ostringstream s;
    passes::Printer(s, true).run(sharedPtr<Node>());
    return s.str();
}

shared_ptr<Statement> Statement::successor() const
{
    return _successor;
}

void Statement::setSuccessor(shared_ptr<Statement> stmt)
{
    _successor = stmt;
}

shared_ptr<Statement> Statement::firstNonBlock()
{
    return sharedPtr<Statement>();
}

statement::Instruction::Instruction(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l)
    : Statement(l)
{
    _id = id;

    addChild(_id);

    _ops = ops;
    for ( auto op : ops )
        addChild(op);

    while ( _ops.size() < 4 )
        _ops.push_back(shared_ptr<Expression>());
}

string statement::Instruction::signature() const
{
    std::list<string> ops;

    auto op = _ops.begin();

    string target;

    if ( op != _ops.end() ) {
        if ( *op )
            target = util::fmt(" -> %s", (*op)->type()->render().c_str());

        while ( ++op != _ops.end() ) {
            if ( *op ) {
                auto o = (*op)->type()->render();
                ops.push_back(o);
            }
        }
    }

    return util::fmt("%s: %s%s", _id->name().c_str(), util::strjoin(ops, ", ").c_str(), target.c_str());
}

void statement::Block::addStatement(shared_ptr<Statement> stmt)
{
    _addComment(stmt);

    auto n = node_ptr<Statement>(stmt);
    _stmts.push_back(n);
    addChild(n);
}

void statement::Block::addStatementAtFront(shared_ptr<Statement> stmt)
{
    _addComment(stmt);

    auto n = node_ptr<Statement>(stmt);
    _stmts.push_front(n);
    addChild(n);
}

void statement::Block::addStatements(const stmt_list& stmts)
{
    for ( auto s : stmts )
        addStatement(s);
}

void statement::Block::addDeclaration(shared_ptr<Declaration> decl)
{
    _addComment(decl);

    auto n = node_ptr<Declaration>(decl);
    _decls.push_back(n);
    addChild(n);
}

void statement::Block::addDeclarations(const decl_list& decls)
{
    for ( auto d : decls )
        addDeclaration(d);
}

void statement::Block::_init(shared_ptr<Scope> parent, shared_ptr<ID> id, const decl_list& decls, const stmt_list& stmts, const Location& l)
{
    _id = id;
    _stmts = stmts;
    _decls = decls;
    _scope = std::make_shared<Scope>(parent);

    if ( _id )
        addChild(_id);

    for ( auto i : stmts )
        addChild(i);

    for ( auto i : decls )
        addChild(i);
}

void statement::Block::_addComment(shared_ptr<Node> node)
{
    if ( _next_comment == "" )
        return;

    node->addComment(_next_comment);
    _next_comment = "";
}

bool statement::Block::terminated() const
{
    if ( ! _stmts.size() )
        return false;

    shared_ptr<Statement> s = _stmts.back();
    auto b = ast::as<statement::Block>(s);

    if ( b )
        return b->terminated();

    auto i = ast::as<statement::instruction::Resolved>(s);

    if ( ! i )
        return false;

    return i->instruction()->terminator();
}

void statement::Block::removeUseless()
{
    stmt_list nstmts;

    for ( auto s : _stmts ) {
        auto i = ast::tryCast<statement::instruction::Resolved>(s);

        if ( i && ast::isA<instruction::Misc::Nop>(i) )
            continue;

        nstmts.push_back(s);

        if ( i && i->instruction()->terminator() )
            break;
    }

    setStatements(nstmts);
}

void statement::Block::setStatements(stmt_list stmts)
{
    for ( auto s : _stmts )
        removeChild(s);

    _stmts = stmts;

    for ( auto s : _stmts )
        addChild(s);
}

bool statement::Block::nop()
{
    assert(false && "not implemented.");
    return false;
}

shared_ptr<Statement> statement::Block::firstNonBlock()
{
    std::set<shared_ptr<statement::Block>> done;
    return _firstNonBlock(sharedPtr<statement::Block>(), &done);
}

shared_ptr<Statement> statement::Block::_firstNonBlock(shared_ptr<statement::Block> block, std::set<shared_ptr<statement::Block>>* done)
{
    for ( auto s : block->statements() ) {
        auto b = ast::tryCast<statement::Block>(s);

        if ( ! b )
            return s;

        if ( done->find(b) != done->end() )
            continue;

        done->insert(b);

        auto first = _firstNonBlock(b, done);
        if ( first )
            return first;
    }

    return nullptr;
}

Statement::FlowInfo statement::Block::flowInfo()
{
    FlowInfo fi;

    for ( auto stmt : _stmts ) {

        auto sfi = stmt->flowInfo();

        auto killed = ::util::set_union(fi.defined, fi.cleared);

        auto modified = ::util::set_difference(sfi.modified, killed);
        auto read     = ::util::set_difference(sfi.read, killed);

        fi.modified = ::util::set_union(fi.modified, modified);
        fi.read     = ::util::set_union(fi.read, read);

        fi.defined = ::util::set_union(fi.defined, sfi.defined);
        fi.cleared = ::util::set_union(fi.cleared, sfi.cleared);

        std::set<shared_ptr<statement::Block>> succ;

        for ( auto s : sfi.successors ) {
            // Ignore successors which are childs of us, we're only
            // interested in those leaving this block.
            if ( ! hasChild(s, true) )
                succ.insert(s);
        }

        fi.successors = ::util::set_union(fi.successors, succ);
    }

    return fi;
}

statement::instruction::Resolved::Resolved(shared_ptr<hilti::Instruction> instruction, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(instruction->id(), ops, l) {
           _instruction = instruction;
       }

Statement::FlowInfo statement::instruction::Resolved::flowInfo()
{
    auto ins = instruction();
    assert(ins);

    FlowInfo fi;

    if ( target() ) {

        variable_set vars;

        for ( auto e :target()->flatten() )
            _addExpressionToVariables(&vars, e);

        fi.defined = util::set_union(fi.defined, vars);
    }

    auto ops = operands();

    int i = 0;
    for ( auto op : ops ) {

        if ( i == 0 || ! op ) {
            ++i;
            continue;
        }

        variable_set vars;

        for ( auto e : op->flatten() )
            _addExpressionToVariables(&vars, e);

        if ( ins->typeOperand(i++).second )
            // Constant.
            fi.read = util::set_union(fi.read, vars);
        else
            // Non-constant.
            fi.modified = util::set_union(fi.modified, vars);
    }

    auto block = firstParent<statement::Block>();

    if ( block )
        fi.successors = instruction()->successors(ops, block->scope());

    return ins->flowInfo(fi);
}

statement::instruction::Unresolved::Unresolved(shared_ptr<ID> id, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(id, ops, l) {}

statement::instruction::Unresolved::Unresolved(const ::string& name, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(shared_ptr<ID>(new ID(name)), ops, l) {}

statement::instruction::Unresolved::Unresolved(shared_ptr<hilti::Instruction> instr, const hilti::instruction::Operands& ops, const Location& l)
    : Instruction(instr->id(), ops, l) { _instr = instr; }

Statement::FlowInfo statement::instruction::Unresolved::flowInfo()
{
    fprintf(stderr, "internal error: Unresolved::flowInfo() called, not all instructions were resolved (%s)\n", id()->name().c_str());
    abort();
}

statement::try_::Catch::Catch(shared_ptr<Type> type, shared_ptr<ID> id, shared_ptr<Block> block, const Location& l)
    : Node(l)
{
    _id = id;
    _block = block;

    addChild(_id);
    addChild(_block);

    if ( id ) {
        auto var = std::make_shared<variable::Local>(id, type, nullptr, l);
        _decl = std::make_shared<declaration::Variable>(id, var, l);
        addChild(_decl);

        // Don't addChild type here, the variable will do it.
    }

    else {
        _type = type;
        addChild(_type);
    }
}

shared_ptr<variable::Local> statement::try_::Catch::Catch::variable() const
{
    return _decl ? ast::as<variable::Local>(_decl->variable()) : nullptr;
}

statement::Try::Try(shared_ptr<Block> try_, const catch_list& catches, const Location& l)
    : Statement(l)
{
    _try = try_;
    _catches = catches;

    addChild(try_);

    for ( auto c : _catches )
        addChild(c);
}

Statement::FlowInfo statement::Try::flowInfo()
{
    auto fi = _try->flowInfo();

    for ( auto c : _catches ) {
        auto cfi = c->block()->flowInfo();

        // Can't adapt cleared/defined here as we don't know whehterh catch
        // will execute. Need to be conservative.

        fi.modified = ::util::set_union(fi.modified, cfi.modified);
        fi.read = ::util::set_union(fi.read, cfi.read);
    }

    return fi;
}

statement::ForEach::ForEach(shared_ptr<ID> id, shared_ptr<Expression> seq, shared_ptr<Block> body, const Location& l)
    : Statement(l)
{
    _id = id;
    _seq = seq;
    _body = body;

    addChild(_seq);
    addChild(_body);
}

shared_ptr<ID> statement::ForEach::ForEach::id() const
{
    return _id;
}

Statement::FlowInfo statement::ForEach::flowInfo()
{
    auto id = _body->scope()->lookup(_id);
    assert(id.size() == 1);
    variable_set iter;

    _addExpressionToVariables(&iter, id.front());

    variable_set vars;

    for ( auto e : _seq->flatten() )
        _addExpressionToVariables(&vars, e);

    FlowInfo fi = _body->flowInfo();
    fi.cleared = ::util::set_union(fi.defined, iter);
    fi.read = ::util::set_union(fi.read, vars);
    fi.read = ::util::set_difference(fi.read, iter);
    // fi.modified = ::util::set_difference(fi.modified, iter);

    return fi;
}

