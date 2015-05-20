
import karabo.hash
from karabo import hashtypes
from karabo.enums import AccessLevel, AccessMode, Assignment
from karabo.hashtypes import (Descriptor, Schema_ as Schema,  # this is not nice
                              Attribute)
from karabo.registry import Registry

from asyncio import coroutine
from enum import Enum
from functools import partial
from collections import OrderedDict


class MetaConfigurable(type(Registry)):
    @staticmethod
    def __prepare__(name, bases):
        return OrderedDict()


class Configurable(Registry, metaclass=MetaConfigurable):
    """The base class for everything that has Karabo attributes.

    A class with Karabo attributes inherits from this class.
    The attributes are then defined as follows:

    ::
        from karabo import Configurable, Int, String

        class Spam(Configurable):
            ham = Int()
            eggs = String()
    """
    _subclasses = { }
    schema = None

    def __init__(self, configuration={}, parent=None, key=None):
        """initialize this object with the configuration

        The configuration is dict or a Hash which contains the
        initial values to be used for the Karabo attributes.
        Default values of attributes are handled here.

        parent is the Configurable object this object belongs to,
        and key is the name of the attribute used in this object.
        They are None for top-layer objects.

        Top-layer objects like Devices are always called with one
        parameter (configuration) only, so if you are inheriting from
        this class you only need to have parent and key as a parameter
        if your class should be usable as a component in another class.
        """
        self._parent = parent
        self._key = key
        for k in self._allattrs:
            t = getattr(type(self), k)
            if k in configuration:
                v = configuration[k]
                if t.enum is not None:
                    v = t.enum(v)
                t.setter(self, v)
                del configuration[k]
            else:
                if t.defaultValue is not None:
                    t.setter(self, t.defaultValue)

    @classmethod
    def register(cls, name, dict):
        super(Configurable, cls).register(name, dict)
        for k, v in dict.items():
            if isinstance(v, Descriptor):
                v.key = k
        cls._attrs = [k for k in dict
                      if isinstance(getattr(cls, k), Descriptor)]
        cls._allattrs = set.union(*(set(c._attrs) for c in cls.__mro__
                                    if hasattr(c, "_attrs")))
        cls.xsd = cls.getClassSchema()
        cls._subclasses = { }
        for b in cls.__bases__:
            if issubclass(b, Configurable):
                b._subclasses[name] = cls

    @classmethod
    def getClassSchema(cls, rules=None):
        schema = karabo.schema.Schema(cls.__name__, hash=rules)
        for c in cls.__mro__[::-1]:
            if hasattr(c, "expectedParameters"):
                c.expectedParameters(schema)
            if hasattr(c, "_attrs"):
                for k in c._attrs:
                    v = getattr(c, k)
                    schema.hash[k] = v.subschema()
                    schema.hash[k, ...] = {
                        k: v.value if isinstance(v, Enum) else v
                        for k, v in v.parameters().items()}
        return schema

    @classmethod
    @coroutine
    def getClassSchema_async(cls, rules=None):
        return cls.getClassSchema(rules)

    def __dir__(self):
        """Return all attributes of this object.

        This is mostly for tab-expansion in IPython."""
        return list(self._allattrs)

    @classmethod
    def node(cls, **kwargs):
        return Node(cls, **kwargs)

    @classmethod
    def choiceOfNodes(cls, **kwargs):
        return ChoiceOfNodes(cls, **kwargs)

    @classmethod
    def listOfNodes(cls, **kwargs):
        return ListOfNodes(cls, **kwargs)

    def setValue(self, descriptor, value):
        if self._parent is not None:
            self._parent.setChildValue(self._key + "." + descriptor.key, value)
        self.__dict__[descriptor.key] = value

    def setChildValue(self, key, value):
        if self._parent is not None:
            self._parent.setChildValue(self._key + "." + key, value)

    def run(self):  # endpoint for multiple inheritance
        self.running = True


class Node(Descriptor):
    """Compose configurable classes into each other

    with a Node you can use a Configurable object in another one as an
    attribute.

    ::

        from karabo import Configurable, Node

        class Outer(Configurable):
            inner = Node(Inner)
    """
    defaultValue = karabo.hash.Hash()

    def __init__(self, cls, **kwargs):
        self.cls = cls
        Descriptor.__init__(self, **kwargs)

    def parameters(self):
        ret = super(Node, self).parameters()
        ret["nodeType"] = 1
        return ret

    def subschema(self):
        return self.cls.getClassSchema().hash

    def __set__(self, instance, value):
        instance.setValue(self, self.cls(value, instance, self.key))

    def asHash(self, instance):
        r = karabo.hash.Hash()
        for k in self.cls._allattrs:
            a = getattr(instance, k, None)
            if a is not None:
                r[k] = getattr(self.cls, k).asHash(a)
        return r


class ChoiceOfNodes(Node):
    defaultValue = Attribute()

    def parameters(self):
        ret = super(ChoiceOfNodes, self).parameters()
        ret["nodeType"] = 2
        return ret

    def subschema(self):
        h = karabo.hash.Hash()
        for k, v in self.cls._subclasses.items():
            h[k] = v.getClassSchema().hash
            h[k, "nodeType"] = 1
        return h

    def __set__(self, instance, value):
        for k, v in value.items():
            instance.setValue(self, self.cls._subclasses[k](v))
            break  # there should be only one entry

    def asHash(self, instance):
        r = karabo.hash.Hash()
        t = type(instance)
        for k in t._allattrs:
            a = getattr(instance, k, None)
            if a is not None:
                r[k] = getattr(t, k).asHash(a)
        return karabo.hash.Hash(t.__name__, r)


class ListOfNodes(Node):
    defaultValue = Attribute()

    def parameters(self):
        ret = super(ListOfNodes, self).parameters()
        ret["nodeType"] = 3
        return ret

    def subschema(self):
        h = karabo.hash.Hash()
        for k, v in self.cls._subclasses.items():
            h[k] = v.getClassSchema().hash
            h[k, "nodeType"] = 1
        return h

    def __set__(self, instance, value):
        if isinstance(value, karabo.hash.Hash):
            l = [self.cls._subclasses[k](v, instance, self.key)
                 for k, v in value.items()]
        else:
            l = [self.cls._subclasses[k](karabo.hash.Hash(),
                    instance, self.key)
                 for k in value]
        instance.setValue(self, l)

    def asHash(self, instance):
        l = []
        for v in instance:
            r = karabo.hash.Hash()
            t = type(v)
            for k in t._allattrs:
                r[k] = getattr(t, k).asHash(getattr(v, k))
            l.append(r)
        return karabo.hash.Hash(self.key, l)


class Validator(object):
    def __init__(self, injectDefaults=True):
        self.injectDefaults = injectDefaults

    def validate(self, schema, hash, stamp=None):
        """validate the hash to fit schema and return the validated hash

        most importantly, if hash does not contain some parameters, those
        are inserted with their default value"""
        return self.r_validate(schema.hash, hash)

    def r_validate(self, subject, object):
        for k, v, a in subject.iterall():
            assert 'nodeType' in a
            if a['nodeType'] == 3:
                if k in object:
                    object[k] = [[karabo.hash.Hash(kk,
                                                   self.r_validate(v[kk], vv))
                                  for kk, vv in vvv.items()][0]
                                 for vvv in object[k]]
                elif self.injectDefaults:
                    dv = a.get('defaultValue', [])
                    if isinstance(dv, str):
                        dv = dv.split()
                    object[k] = [karabo.hash.Hash(
                                 vv, self.r_validate(v[vv],
                                                     karabo.hash.Hash()))
                                 for vv in dv]
            elif a['nodeType'] == 2:
                if k in object:
                    object[k] = [karabo.hash.Hash(kk,
                                                  self.r_validate(v[kk], vv))
                                 for kk, vv in object[k].items()][0]
                elif self.injectDefaults and "defaultValue" in a:
                    dv = a['defaultValue']
                    object[k] = karabo.hash.Hash(
                        dv, self.r_validate(v[dv], karabo.hash.Hash()))
            elif k in object:
                object[k] = self.r_validate(v, object[k])
            elif self.injectDefaults and 'defaultValue' in a:
                object[k] = a['defaultValue']
            elif a['nodeType'] == 1:
                object[k] = self.r_validate(v, karabo.hash.Hash())
            else:
                pass
                #print 'not validated:', k
        return object
