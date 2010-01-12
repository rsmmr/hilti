# $Id$
# 
# @TEST-EXEC:  hiltic %INPUT -p -o out.ll
# @TEST-EXEC:  test-diff out.ll

module main

import Hilti

struct A {
    ref<A> a,
    ref<B> b
    }

struct B {
    ref<A> a,
    ref<B> b,
    ref<A> c,
    ref<B> d
    }

exception myException(ref<A>)
global ref<A> a
global ref<B> b
global tuple<ref<A>,ref<B>> t
global ref<list<ref<A>>> v

ref<A> foo(ref<B> b) {
}

export void run() {
}
