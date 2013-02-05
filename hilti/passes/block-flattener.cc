
#include "../statement.h"
#include "../module.h"
#include "../hilti-intern.h"
#include "autogen/instructions.h"

#include "block-flattener.h"
#include "printer.h"

using namespace hilti;
using namespace passes;

// A helper pass that renames an ID within a subtree.
class VarRenamer : public Pass<>
{
public:
    VarRenamer(shared_ptr<ID> old, shared_ptr<ID> new_) : Pass<>("hilti::block-flattener::VarRenamer") {
        _old = old;
        _new = new_;
        new_->setOriginal(_old);
    }

    virtual ~VarRenamer() {}

    bool run(shared_ptr<Node> stmt) override {
        return processAllPreOrder(stmt);
    }

protected:
    void visit(ID* i) override {

        if ( i->firstParent<declaration::Type>() )
            return;

        if ( i->pathAsString() == _old->pathAsString() )
            i->replace(_new);
    }

    void visit(expression::ID* i) override {
        if ( i->id()->pathAsString() == _old->pathAsString() )
            i->replace(std::make_shared<expression::ID>(_new, i->location()));
    }

private:
    shared_ptr<ID> _old;
    shared_ptr<ID> _new;
};

BlockFlattener::BlockFlattener() : Pass<>("hilti::codegen::BlockFlattener")
{
}

static shared_ptr<hilti::Node> _module;

bool BlockFlattener::run(shared_ptr<hilti::Node> module)
{
    _module = module;
    return processAllPostOrder(module);
}

shared_ptr<statement::Block> BlockFlattener::flatten(shared_ptr<statement::Block> src, shared_ptr<statement::Block> toplevel)
{
    shared_ptr<statement::Block> first_new_block = nullptr;
    shared_ptr<statement::Block> cur = nullptr;

    auto stmts = src->statements();

    if ( ! stmts.size() ) {
        hilti::instruction::Operands ops = { };
        auto nop = std::make_shared<statement::instruction::Unresolved>(instruction::Misc::Nop, ops);
        stmts = { nop };
    }

    for ( auto s : stmts ) {
        auto b = ast::tryCast<statement::Block>(s);

        if ( b ) {
            auto fb = flatten(b, toplevel);
            assert(fb);

            if ( ! first_new_block ) {
                first_new_block = fb;

                if ( src->id() )
                    fb->setID(src->id());
            }

            cur = nullptr;
            continue;
        }

        if ( ! cur ) {
            cur = std::make_shared<statement::Block>(nullptr);
            toplevel->addStatement(cur);
            cur->scope()->setParent(toplevel->scope());

            if ( ! first_new_block ) {
                first_new_block = cur;

                if ( src->id() )
                    cur->setID(src->id());
            }
        }

        cur->addStatement(s);
    }

    for ( auto d : src->declarations() ) {
        auto orig_id = d->id();
        int cnt = 1;
        bool rename = false;

        auto var = ast::tryCast<declaration::Variable>(d);

        if ( var && ast::isA<variable::Local>(var->variable()) ) {
            // Rename local if we already have defined another one under the
            // same name.
            while ( toplevel->scope()->has(d->id()) ) {

                auto d2 = toplevel->scope()->lookupUnique(d->id());
                if ( ast::checkedCast<expression::Variable>(d2)->variable() == var->variable() )
                    break;

                auto name = ::util::fmt("%s_%d", orig_id->name(), ++cnt);
                d->setID(std::make_shared<ID>(name, d->id()->location()));
                rename = true;
            }

            if ( rename ) {
                VarRenamer renamer(orig_id, d->id());
                renamer.run(src);
            }
        }

        toplevel->addDeclaration(d);

        if ( var ) {
            toplevel->scope()->remove(d->id());
            toplevel->scope()->insert(d->id(), std::make_shared<expression::Variable>(var->variable()));
        }
    }

    toplevel->scope()->mergeFrom(src->scope());

#if 0
    std::ostringstream s;
    passes::Printer(s).run(_module);
    std::cerr << s.str().c_str() << std::endl;
    std::cerr << "----------------------------" << std::endl;
#endif

    return first_new_block;
}

void BlockFlattener::visit(declaration::Function* d)
{
    auto func = d->function();

    if ( ! func->body() )
        return;

    auto obody = ast::checkedCast<statement::Block>(func->body());
    auto nbody = std::make_shared<statement::Block>(nullptr);
    nbody->setScope(obody->scope());
    flatten(obody, nbody);
    func->setBody(nbody);
}

void BlockFlattener::visit(Module* m)
{
    auto obody = ast::checkedCast<statement::Block>(m->body());
    auto nbody = std::make_shared<statement::Block>(nullptr);
    nbody->setScope(obody->scope());
    flatten(obody, nbody);
    m->setBody(nbody);
}
