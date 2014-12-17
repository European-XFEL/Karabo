
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
    _subclasses = []
    schema = None

    def __init__(self, configuration={}, parent=None, key=None):
        self._parent = parent
        self._key = key
        for k in self._allattrs:
            t = getattr(type(self), k)
            if k in configuration:
                v = configuration[k]
                if t.enum is not None:
                    v = t.enum(v)
                t.method(self, v)
                del configuration[k]
            else:
                if t.defaultValue is not None:
                    t.method(self, t.defaultValue)

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
        cls._subclasses = []
        for b in cls.__bases__:
            if issubclass(b, Configurable):
                b._subclasses.append(cls)

    @classmethod
    def getClassSchema(cls, rules=None):
        schema = karabo.schema.Schema(cls.__name__, rules)
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

    def setValue(self, key, value):
        if self._parent is not None:
            self._parent.setChildValue(self._key + "." + key.key, value)
        self.__dict__[key] = value

    def setChildValue(self, key, value):
        self._parent.setChildValue(self._key + "." + key, value)

    def run(self):  # endpoint for multiple inheritance
        self.running = True


class Node(Descriptor):
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
        for c in self.cls._subclasses:
            h[c.__name__] = c.getClassSchema().hash

    def __set__(self, instance, value):
        for k, v in value.items():
            for c in self.cls._subclasses:
                if c.__name__ == k:
                    instance.setValue(self, c(value))
                    break
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
        for c in self.cls._subclasses:
            h[c.__name__] = c.getClassSchema().hash
        return h

    def __set__(self, instance, value):
        l = []
        for k, v in value.items():
            for c in self.cls._subclasses:
                if c.__name__ == k:
                    l.append(c(value, instance, self.key))
                    break
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
