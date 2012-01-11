
#ifndef AST_MIXIN_H
#define AST_MIXIN_H

namespace ast {

/// Base class for mixins.
template<typename Target, typename Overrider>
class MixIn {
public:
   MixIn(Target* target, Overrider* overrider) {
       _target = target;
       target->setOverrider(overrider);
       overrider->setObject(target);
   }

   Target* mixinTarget() const { return _target; }

private:
   Target* _target;
};

template<typename O>
class Overridable {
public:
   Overridable()                     { _overrider = &_default_overrider; }
   O* overrider() const              { return _overrider; }
   void setOverrider(O* overrider)   { _overrider = overrider; }

private:
   O* _overrider;
   O  _default_overrider;

};

template<typename Object>
class Overrider {
public:
   shared_ptr<Object> object() const { assert(_object); return _object->template sharedPtr<Object>(); }
   void setObject(Object* object) { _object = object; }

private:
   Object* _object;
};

}

#endif
