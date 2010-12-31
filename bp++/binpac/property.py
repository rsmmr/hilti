# $Id$
#
# Defines a class storing values for a set of predefined properties.

import util

class Container:
    def __init__(self):
        """Creates a new property container.
        """
        self._props = {}

    def allProperties(self):
        """Returns a list of all recognized properties.

        Can be overwritten by derived classes. The default implementation
        returns an empty dictionary.

        Returns: dict mapping string to ~~Ctor or list of ~~Ctor - For each
        allowed property, there is one entry under it's name (excluding the
        leading dot) mapping to its default value. If the default value is a
        list, it is assumed that the property can have multiple values. 
        """
        return {}

    def property(self, name, parent=None):
        """Returns the value of property. The property name must be one of
        those returned by ~~allProperties, or defined by the module.

        name: string - The name of the property. It's an error to pass in a
        name that cannot be resolved to any known property.

        parent: Container - If given, if the property is not defined by the
        unit (or it's value evaluates to False), the parent is checked.

        Returns: ~~Ctor or list of ~Ctor - The value of the property. If not explicity
        set, the default value is returned. The returned constant will be of
        the same type as that of the default value returned by ~~allProperties.
        """
        try:
            default = self.allProperties()[name]
        except KeyError:
            if parent:
                return parent.property(name, True)

            util.internal_error("unknown property '%s'" % name)

        return self._props.get(name, default)

    def setProperty(self, name, constant):
        """Sets the value of a property. The property name must be one of
        those returned by ~~allProperties. If the name's default value is a
        list, it is assumed that the property can have more than one value,
        and each call to ~~setProperty adds one element to the list (if not
        already a member). 

        name: string - The name of the property.
        value: any - The value to set it to. The type must correspond to what
        ~~allProperties specifies for the default value.

        Raises: ValueError if ``name`` is not a valid property.
        """
        try:
            default = self.allProperties()[name]
        except:
            raise ValueError(name)

        if not isinstance(default, list):
            self._props[name] = constant
        else:
            self._props[name] = self._props.get(name, default) + [constant]

    def resolveProperties(self, resolver):
        """XXXX"""
        for e in self._props.values():
            if not e:
                continue

            if isinstance(e, list):
                for c in e:
                    if c:
                        c.resolve(resolver)
            else:
                e.resolve(resolver)

