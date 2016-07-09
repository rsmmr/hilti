
#include "rtti.h"

ast::rtti::RTTI::ID ast::rtti::RTTI::_id_counter = 0;
bool ast::rtti::RTTI::_initialized = false;
std::unique_ptr<std::list<ast::rtti::RTTI*>> ast::rtti::RTTI::_all;

void ast::rtti::RTTI::init()
{
    if ( _initialized )
        return;

    if ( ! _all )
        return;

    for ( auto i : *_all ) {
        i->_init();
        // fprintf(stderr, "%s -> %s\n", i->_name, i->_mask.to_string().c_str());
    }

    _initialized = true;
}

void ast::rtti::RTTI::_init()
{
    if ( _mask.any() )
        return;

    assert(_id > 0);
    _mask[_id] = 1;

    for ( auto p : _parents ) {
        p->_init();
        _mask |= p->_mask;
    }
}
