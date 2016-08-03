from asyncio import coroutine, gather
from collections import OrderedDict
from enum import Enum

from .basetypes import KaraboValue
from .enums import NodeType
from .hash import Attribute, Descriptor, Hash, Schema
from .registry import Registry
from .timestamp import Timestamp
from .weak import Weak


class MetaConfigurable(type(Registry)):
    @staticmethod
    def __prepare__(name, bases):
        return OrderedDict()


class Configurable(Registry, metaclass=MetaConfigurable):
    """The base class for everything that has Karabo attributes.

    A class with Karabo attributes inherits from this class.
    The attributes are then defined as follows::

        from karabo.middlelayer import Configurable, Int32, String

        class Spam(Configurable):
            ham = Int32()
            eggs = String()
    """
    _subclasses = { }
    __parent = Weak()
    schema = None

    def __init__(self, configuration={}, parent=None, key=None):
        """Initialize this object with the configuration

        :param configuration: a :class:`dict` or a
        :class:`~karabo.middlelayer.Hash` which contains the
        initial values to be used for the Karabo attributes.
        Default values of attributes are handled here.

        :param parent: the :class:`Configurable` object this object
        belongs to
        :param key: the name of the attribute used in this object.

        Top-layer objects like devices are always called with one
        parameter (configuration) only, so if you are inheriting from
        this class you only need to have ``parent`` and ``key`` as a
        parameter if your class should be usable as a component in
        another class.
        """

        self.__parent = parent
        self.__key = key
        self._initializers = []
        for k in self._allattrs:
            t = getattr(type(self), k)
            init = []
            if k in configuration:
                v = configuration[k]
                init = t.checkedInit(self, v)
                del configuration[k]
            else:
                init = t.checkedInit(self)
            self._initializers.extend(init)

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
        cls._subclasses = { }
        for b in cls.__bases__:
            if issubclass(b, Configurable):
                b._subclasses[name] = cls

    @classmethod
    def getClassSchema(cls, device=None, state=None):
        schema = Schema(cls.__name__)
        for base in cls.__mro__[::-1]:
            for attr in getattr(base, "_attrs", []):
                descr = getattr(base, attr)
                if (state is None or descr.allowedStates is None or
                        state in descr.allowedStates):
                    sub_schema, attrs = descr.toSchemaAndAttrs(device, state)
                    schema.hash[attr] = sub_schema
                    schema.hash[attr, ...] = attrs
        return schema

    def getDeviceSchema(self, state=None):
        return self.getClassSchema(self, state)

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
        if isinstance(value, KaraboValue) and value.timestamp is None:
            value.timestamp = Timestamp()
        self.setChildValue(descriptor.key, value, descriptor)
        self.__dict__[descriptor.key] = value

    def setChildValue(self, key, value, desc):
        if self.__parent is not None:
            self.__parent.setChildValue(self.__key + "." + key, value, desc)

    @coroutine
    def slotReconfigure(self, config):
        props = ((getattr(self.__class__, k), v) for k, v in config.items())
        setters = sum((t.checkedSet(self, v) for t, v in props), [])
        setters = (s() for s in setters)
        yield from gather(*[s for s in setters if s is not None])

    @coroutine
    def _run(self):
        """ post-initialization of a Configurable

        As __init__ is not a coroutine, it cannot do anything that takes
        time, and actually should not anyways. Everything needed to get
        this configurable running is thus done in this method.

        This method should only be overridden inside Karabo (thus
        the underscore). Do not forget to ``yield from super()._run()``.
        """
        yield from gather(*self._initializers)
        del self._initializers

    def _use(self):
        """this method is called each time an attribute of this configurable
        is read"""


class Node(Descriptor):
    """Compose configurable classes into each other

    with a Node you can use a Configurable object in another one as an
    attribute::

        from karabo import Configurable, Node

        class Outer(Configurable):
            inner = Node(Inner)
    """
    defaultValue = Hash()

    def __init__(self, cls, **kwargs):
        self.cls = cls
        Descriptor.__init__(self, **kwargs)

    def toSchemaAndAttrs(self, device, state):
        _, attrs = super(Node, self).toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.Node
        return self.cls.getClassSchema(device, state).hash, attrs

    def _initialize(self, instance, value):
        node = self.cls(value, instance, self.key)
        instance.__dict__[self.key] = node
        ret = node._initializers
        del node._initializers
        return ret

    def _setter(self, instance, value):
        props = ((getattr(self.cls, k), v) for k, v in value.items())
        parent = getattr(instance, self.key)
        return sum((t.checkedSet(parent, v) for t, v in props), [])

    def toDataAndAttrs(self, instance):
        r = Hash()
        for k in self.cls._allattrs:
            a = getattr(instance, k, None)
            if a is not None:
                value, attrs = getattr(self.cls, k).toDataAndAttrs(a)
                r[k] = value
                r[k, ...].update(attrs)
        return r, {}


class ChoiceOfNodes(Node):
    defaultValue = Attribute()

    def toSchemaAndAttrs(self, device, state):
        h = Hash()
        for k, v in self.cls._subclasses.items():
            h[k] = v.getClassSchema(device, state).hash
            h[k, "nodeType"] = NodeType.Node

        _, attrs = super().toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.ChoiceOfNodes
        return h, attrs

    def _initialize(self, instance, value):
        for k, v in value.items():  # there should be only one entry
            node = self.cls._subclasses[k](v)
            instance.__dict__[self.key] = node
            ret = node._initializers
            del node._initializers
            return ret

    def toDataAndAttrs(self, instance):
        r = Hash()
        t = type(instance)
        for k in t._allattrs:
            a = getattr(instance, k, None)
            if a is not None:
                value, attrs = getattr(t, k).toDataAndAttrs(a)
                r[k] = value
                r[k, ...].update(attrs)
        return Hash(t.__name__, r), {}


class ListOfNodes(Node):
    defaultValue = Attribute()

    def toSchemaAndAttrs(self, device, state):
        h = Hash()
        for k, v in self.cls._subclasses.items():
            h[k] = v.getClassSchema(device, state).hash
            h[k, "nodeType"] = NodeType.Node

        _, attrs = super(ListOfNodes, self).toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.ListOfNodes
        return h, attrs

    def _initialize(self, instance, value):
        if isinstance(value, Hash):
            l = [self.cls._subclasses[k](v, instance, self.key)
                 for k, v in value.items()]
        else:
            l = [self.cls._subclasses[k](Hash(), instance, self.key)
                 for k in value]
        instance.__dict__[self.key] = l
        ret = []
        for o in l:
            ret += o._initializers
            del o._initializers
        return ret

    def toDataAndAttrs(self, instance):
        l = []
        for v in instance:
            r = Hash()
            t = type(v)
            for k in t._allattrs:
                value, attrs = getattr(t, k).toDataAndAttrs(getattr(v, k))
                r[k] = value
                r[k, ...] = attrs
            l.append(r)
        return Hash(self.key, l), {}
