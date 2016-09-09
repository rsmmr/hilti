// Custom RTTI for dynamic type-checking and casting.

#ifndef AST_RTTI_H
#define AST_RTTI_H

#include <bitset>
#include <cassert>
#include <list>
#include <memory>

#include <stdio.h>

// Helpers.
#define __combine1(x, y) x##y
#define __combine(x, y) __combine1(x, y)

namespace ast {
class NodeBase;
}

// Macros to add RTTI information to classes.
#define AST_RTTI                                                                                   \
public:                                                                                            \
    const ::ast::rtti::RTTI& rtti() const override                                                 \
    {                                                                                              \
        return _rtti;                                                                              \
    }                                                                                              \
    static ::ast::rtti::RTTI* __rtti()                                                             \
    {                                                                                              \
        return &_rtti;                                                                             \
    }                                                                                              \
    const void* __rtti_cast(const ::ast::rtti::RTTI&) const override;                              \
                                                                                                   \
private:                                                                                           \
    static ::ast::rtti::RTTI _rtti;

#define AST_RTTI_BEGIN(cls, id)                                                                    \
    ::ast::rtti::RTTI cls::_rtti;                                                                  \
    class __combine(__RTTIImpl_, id)                                                               \
    {                                                                                              \
    public:                                                                                        \
        __combine(__RTTIImpl_, id)()                                                               \
        {                                                                                          \
            ::ast::rtti::RTTI* rtti = cls::__rtti();                                               \
            rtti->setName(#cls);

#define AST_RTTI_PARENT(p) rtti->setParent(p::__rtti());

#define AST_RTTI_END(id)                                                                           \
    }                                                                                              \
    }                                                                                              \
    __combine(__rtti_impl_, id);

// Safe casting across hierarchies. See http://www.cs.rug.nl/~alext/SOFTWARE/RTTI/node17.html
#define AST_RTTI_CAST_BEGIN(cls)                                                                   \
    const void* cls::__rtti_cast(const ::ast::rtti::RTTI& r) const                                 \
    {                                                                                              \
        if ( r.id() == cls::__rtti()->id() )                                                       \
            return this;

#define AST_RTTI_CAST_PARENT(p)                                                                    \
    if ( auto q = p::__rtti_cast(r) )                                                              \
        return q;

#define AST_RTTI_CAST_END(p)                                                                       \
    return 0;                                                                                      \
    }

namespace ast {
namespace rtti {

class RTTI {
public:
    static const int MAX_CLASSES = 2048;
    typedef unsigned int ID;
    typedef std::bitset<MAX_CLASSES> Mask;

    RTTI() : _id(++_id_counter), _mask(0), _name(0)
    {
        if ( ! _all )
            _all = std::unique_ptr<std::list<RTTI*>>(new std::list<RTTI*>);

        _all->push_back(this);
    }

    const char* name() const
    {
        assert(_initialized);
        return _name;
    }
    ID id() const
    {
        assert(_initialized);
        return _id;
    }
    const Mask& mask() const
    {
        assert(_initialized);
        return _mask;
    }

    void setParent(RTTI* parent)
    {
        assert(! _initialized);
        _parents.push_back(parent);
        // fprintf(stderr, "    %s (%u)\n", parent->_name, parent->_id);
    }

    void setName(const char* name)
    {
        assert(! _initialized);
        _name = name;
        // fprintf(stderr, "%s (%u)\n", _name, _id);
    }

    /// Initialize all RTTI classes. Must be called once at startup before
    /// any RTTI information can be used.
    static void init();

    /// Returns true if intialized has been called and completed.
    static bool intialized()
    {
        return _initialized;
    }

private:
    void _init();

    ID _id;
    Mask _mask;
    const char* _name;
    std::list<RTTI*> _parents;

    static ID _id_counter;
    static bool _initialized;
    static std::unique_ptr<std::list<RTTI*>> _all;
};


class Base {
public:
    virtual ~Base()
    {
    }
    virtual const RTTI& rtti() const = 0;
    virtual const void* __rtti_cast(const ::ast::rtti::RTTI&) const = 0;
};

template <typename T>
inline const char* typeName(T& t)
{
    return t.rtti().name();
}

template <typename T>
inline const char* typeName(T* t)
{
    assert(t);
    return t->rtti().name();
}

template <typename T>
inline const char* typeName()
{
    return T::__rtti()->name();
}

template <typename T>
inline RTTI::ID typeId(T& t)
{
    return t.rtti().id();
}

template <typename T>
inline RTTI::ID typeId(T* t)
{
    assert(t);
    return t->rtti().id();
}

template <typename T>
inline RTTI::ID typeId(std::shared_ptr<T> t)
{
    assert(t);
    return t->rtti().id();
}

template <typename T>
inline RTTI::ID typeId()
{
    return T::__rtti()->id();
}

template <typename D, typename S>
inline bool isA(S s, bool ok_on_init = false)
{
    assert(s);

    if ( ok_on_init && ! RTTI::intialized() )
        return true;

    return s->rtti().mask()[D::__rtti()->id()];
}

template <typename D, typename S>
D* tryCast(S* s)
{
    if ( ! s )
        // Null is ok.
        return nullptr;

    if ( ! isA<D>(s) )
        // Use the quick check first.
        return nullptr;

    return reinterpret_cast<D*>(const_cast<void*>(s->__rtti_cast(*D::__rtti())));
}

template <typename D, typename S>
const D* tryCast(const S* s)
{
    if ( ! s )
        // Null is ok.
        return nullptr;

    if ( ! isA<D>(s) )
        // Use the quick check first.
        return nullptr;

    return reinterpret_cast<const D*>(s->__rtti_cast(*D::__rtti()));
}

template <typename D, typename S>
D* checkedCast(S* s)
{
    if ( ! s )
        // Null is ok.
        return nullptr;

    if ( auto d = tryCast<D>(s) )
        return d;

    fprintf(stderr, "internal error: ast::rtti::checkedCast() failed; wanted '%s' but got a '%s'\n",
            typeName<D>(), typeName(s));

    fprintf(stderr, "debug: isA<D>(s): %d\n", isA<D>(s));
    abort();
}

// For nodes, we also support casting from shared_ptrs.

template <typename D>
std::shared_ptr<D> tryCast(std::shared_ptr<NodeBase> n)
{
    if ( ! n )
        // Null is ok.
        return nullptr;

    if ( auto d = tryCast<D>(n.get()) )
        return std::static_pointer_cast<D>(d->shared_from_this());
    else
        return nullptr;
}

template <typename D>
std::shared_ptr<D> checkedCast(std::shared_ptr<NodeBase> n)
{
    if ( ! n )
        // Null is ok.
        return nullptr;

    auto d = checkedCast<D>(n.get());
    return std::static_pointer_cast<D>(d->shared_from_this());
}
}
}

#endif
