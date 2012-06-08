
#include "module.h"
#include "instruction.h"
#include "expression.h"

namespace hilti {
namespace instruction {
namespace classifier {

void _validateRuleValue(const Instruction* i, shared_ptr<Type> rtype, shared_ptr<Expression> op)
{
    fprintf(stderr, "1\n");

    assert(op);

    shared_ptr<Type> stype = nullptr;
    auto ttype = ast::as<type::Tuple>(op->type());

    fprintf(stderr, "2\n");

    if ( ! ttype )
        stype = op->type();

    else {

        fprintf(stderr, "3\n");

        if ( ttype->typeList().size() != 2 ) {
            i->error(ttype, "tuple must be of type (struct, int)");
            return;
        }

        auto j = ttype->typeList().begin();

        stype = *j++;
        auto itype = *j++;

        fprintf(stderr, "4\n");

        fprintf(stderr, "4 %s\n", stype ? stype->render().c_str() : "---");
        fprintf(stderr, "4 %s\n", itype ? itype->render().c_str() : "---");

        if ( ! ast::isA<type::Integer>(itype) ) {
            fprintf(stderr, "4.5\n");
            i->error(ttype, "tuple must be of type (struct, int)");
            return;
        }

        fprintf(stderr, "5\n");
    }

    fprintf(stderr, "6\n");

    assert(stype);

    auto ref = ast::as<type::Reference>(stype);

    if ( ! ref ) {
        i->error(op, "rule value must be a reference to a struct");
        return;
    }

    fprintf(stderr, "7\n");

    auto s = ast::as<type::Struct>(ref->argType());
    auto r = ast::as<type::Struct>(rtype);
    assert(r);

    if ( ! s ) {
        i->error(op, "rule value must be a reference to a struct");
        return;
    }

    if ( r->typeList().size() != s->typeList().size() ) {
        i->error(op, "rule value has wrong number of elements");
        return;
    }

    fprintf(stderr, "7\n");

    auto ri = r->typeList().begin();
    auto si = s->typeList().begin();

    for ( ; ri != r->typeList().end(); ri++, si++ ) {
        if ( (*ri)->equal(*si) )
            continue;

        fprintf(stderr, "8\n");

        bool match = false;

        auto ct = ast::as<type::trait::Classifiable>(*ri);
        assert(ct);

        for ( auto t : ct->alsoMatchableTo() ) {
            if ( i->canCoerceTo(*si, t) )
                match = true;
        }

        fprintf(stderr, "9\n");

        if ( ! match ) {
            i->error(op, "rule value element not compatible with field type");
            return;
        }
    }
}

}
}
}

