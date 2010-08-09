# $Id$

module main

import hilti

type A = struct {
    ref<A> a,
    ref<B> b
}
type B = struct {
    ref<A> a,
    ref<B> b,
    ref<A> c,
    ref<B> d
}

global ref<A> a
global ref<B> b

ref<A> foo(ref<B> b) {
@__b3:
    return.result a
    }


type myException = exception<ref<A>>

void run() {
@__b4:
    }

export run


global tuple<ref<A>,ref<B>> t
global ref<list<ref<A>>> v
