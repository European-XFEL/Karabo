# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from asyncio import gather
from functools import partial
from weakref import WeakKeyDictionary

from karabo.common.api import KARABO_RUNTIME_ATTRIBUTES_MDL
from karabo.native.data import (
    AccessLevel, Hash, HashList, NodeType, Schema, Timestamp, has_changes)
from karabo.native.time_mixin import get_timestamp

from .basetypes import KaraboValue, NoneValue, isSet
from .descriptors import Attribute, Descriptor, Integer, Slot

__all__ = ['Configurable', 'ChoiceOfNodes', 'ListOfNodes', 'Node', 'Overwrite']


def _get_setters(instance, hsh, only_changes=False):
    """Get the list of tuples of decriptor, instance and value

    :param hsh: configuration Hash
    :param only_changes: Boolean if only changed values are considered

    returns: [tuple(descriptor, instance, value), ...]
    """
    if not isinstance(hsh, Hash):
        raise RuntimeError(
            f"Value must be of a `Hash`, got {type(hsh).__name__} instead.")

    setter = []
    for k, v in hsh.items():
        desc = getattr(instance.__class__, k)
        if isinstance(desc, Node):
            node = getattr(instance, k)
            setter.extend(_get_setters(node, v, only_changes))
        else:
            v = desc.toKaraboValue(v)
            if v.timestamp is None:
                # If there is no timestamp, try to get it
                # from attributes
                v.timestamp = Timestamp.fromHashAttributes(
                    hsh[k, ...])
            if only_changes:
                old = getattr(instance, k)
                if not has_changes(old.value, v.value):
                    continue
            setter.append((desc, instance, v))
    return setter


class Configurable:
    """The base class for everything that has Karabo attributes.

    A class with Karabo attributes inherits from this class.
    The attributes are then defined as follows::

        from karabo.native import Configurable, Int32, String

        class Spam(Configurable):
            ham = Int32()
            eggs = String()
    """
    _subclasses = {}
    daqDataType = None
    allowedActions = None
    displayType = None
    classId = None
    schema = None

    def __init__(self, configuration={}):
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
        self._parents = WeakKeyDictionary()
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
    def __init_subclass__(cls, **kwargs):
        super().__init_subclass__(**kwargs)
        for k, v in cls.__dict__.items():
            if isinstance(v, Overwrite):
                setattr(cls, k, v.overwrite(getattr(super(cls, cls), k)))
        cls._attrs = [k for k in cls.__dict__
                      if isinstance(getattr(cls, k), Descriptor)]
        allattrs = []
        seen = set()
        for base in cls.__mro__[::-1]:
            for attr in getattr(base, "_attrs", []):
                if attr not in seen:
                    allattrs.append(attr)
                    seen.add(attr)
        cls._allattrs = allattrs
        cls._subclasses = {}
        for b in cls.__bases__:
            if issubclass(b, Configurable):
                b._subclasses[cls.__name__] = cls

    @classmethod
    def getClassSchema(cls, device=None, state=None):
        schema = Schema(cls.__name__)
        for base in cls.__mro__[::-1]:
            for attr in getattr(base, "_attrs", []):
                descr = getattr(base, attr)
                if (state is None or descr.allowedStates is None or
                        state in descr.allowedStates):
                    sub_schema, attrs = descr.toSchemaAndAttrs(device, state)
                    schema.hash.setElement(attr, sub_schema, attrs)
        return schema

    def get_root(self):
        """Return the root instance of this Configurable"""
        root = self
        while True:
            try:
                root = next(iter(root._parents))
            except StopIteration:
                break

        return root

    def getDeviceSchema(self, state=None):
        return self.getClassSchema(self, state)

    def configurationAsHash(self):
        r = Hash()
        for k in self._allattrs:
            a = getattr(self, k, None)
            if isSet(a):
                v = getattr(type(self), k)
                if isinstance(v, Slot):
                    continue
                value, attrs = v.toDataAndAttrs(a)
                r.setElement(k, value, attrs)
        return r

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
            value.timestamp = get_timestamp()
        elif isinstance(value, KaraboValue) and value.tid == 0:
            value.timestamp = get_timestamp(value.timestamp)
        if isSet(value):
            # calls the setChildValue of the signalslotable
            self.setChildValue(descriptor.key, value, descriptor)
        old = self.__dict__.get(descriptor.key)
        if isinstance(old, Configurable):
            del old._parents[self]
        if isinstance(value, Configurable):
            value._parents[self] = descriptor.key
        self.__dict__[descriptor.key] = value

    def setChildValue(self, key, value, desc):
        """Set all values in Nodes"""
        for parent, parentkey in self._parents.items():
            parent.setChildValue(f"{parentkey}.{key}", value, desc)

    def _getValue(self, key):
        ret = self.__dict__.get(key)
        if ret is None:
            ret = NoneValue(descriptor=getattr(self.__class__, key))
            ret._parent = self
        return ret

    async def slotReconfigure(self, config):
        props = ((getattr(self.__class__, k), v) for k, v in config.items())
        setters = sum((t.checkedSet(self, v) for t, v in props), [])
        setters = (s() for s in setters)
        await gather(*[s for s in setters if s is not None])

    async def set_setter(self, config, only_changes=False):
        """Internal handler to set a Hash on the Configurable via the setter
        functions

        :param only_changes: Boolean to check if only changed values should
                             be set, the default is `False`.
        """
        setters = [partial(desc.setter, instance, value)
                   for (desc, instance, value)
                   in _get_setters(self, config, only_changes)]
        setters = (s() for s in setters)
        await gather(*[s for s in setters if s is not None])

    def set(self, config, only_changes=False):
        """Internal handler to set a Hash on the Configurable

        :param only_changes: Boolean to check if only changed values should
                             be set, the default is `False`.
        """
        setters = _get_setters(self, config, only_changes)
        for desc, instance, value in setters:
            # This is similar to descriptor's `__set__`
            value._parent = instance
            instance.setValue(desc, value)

    async def _run(self):
        """ post-initialization of a Configurable

        As __init__ is not a coroutine, it cannot do anything that takes
        time, and actually should not anyways. Everything needed to get
        this configurable running is thus done in this method.

        This method should only be overridden inside Karabo (thus
        the underscore).

        Do not forget to ``await super()._run(**kwargs)``.
        """
        await gather(*self._initializers)
        del self._initializers

    def applyRuntimeUpdates(self, updates):
        """Apply run-time attribute updates to a schema

        :param updates: List of Hashes with "path", "attribute" and "value"
        :return success: True if all updates could be applied, False otherwise
        """

        def _overwrite_attribute(path, attr_name, attr_value):
            # Basic check if the attribute is allowed to be set
            if attr_name not in KARABO_RUNTIME_ATTRIBUTES_MDL:
                return False

            if len(path.split('.')) > 1:
                return False

            value = getattr(self, path, None)
            if not isSet(value):
                return False

            # Overwrite the attribute
            setattr(self.__class__, path, Overwrite(attr_name, attr_value))
            return True

        success = True
        for update in updates:
            path, attr_name, attr_value = (update["path"], update["attribute"],
                                           update["value"])
            success &= _overwrite_attribute(path, attr_name, attr_value)

        return success

    def _use(self):
        """this method is called each time an attribute of this configurable
        is read"""


class Overwrite:
    """Overwrite the attributes of an inherited property

    This looks like a normal property definition, just that it takes the
    inherited one and updates it::

        class Base(Configurable):
            number = Int32(minExc=3)

        class Child(Base):
            number = Overwrite(minExc=7)
    """

    def __init__(self, *args, **kwargs):
        kwargs.update(zip(args[::2], args[1::2]))
        self.kwargs = kwargs

    def overwrite(self, original):
        _, attrs = original.toSchemaAndAttrs(None, None)
        attrs.pop("enum", None)
        if issubclass(original.__class__, Integer):
            # Integers might have enum values and have to be treated special
            # as their options have to be reinitialized!
            ret = original.__class__(strict=False, enum=original.enum, **attrs)
            ret.setter = original.setter
            ret.__init__(key=original.key, enum=original.enum, **self.kwargs)

            return ret

        if issubclass(original.__class__, Slot):
            ret = original.__class__(strict=False, **attrs)
            ret.method = original.method
        else:
            ret = original.__class__(strict=False,
                                     enum=original.enum, **attrs)
            ret.setter = original.setter
        ret.__init__(key=original.key, **self.kwargs)

        return ret


class Node(Descriptor):
    """Compose configurable classes into each other

    with a Node you can use a Configurable object in another one as an
    attribute::

        from karabo import Configurable, Node

        class Outer(Configurable):
            inner = Node(Inner)
    """
    defaultValue = Hash()

    def __init__(self, cls, requiredAccessLevel=AccessLevel.OBSERVER,
                 **kwargs):
        self.cls = cls
        Descriptor.__init__(self, requiredAccessLevel=requiredAccessLevel,
                            **kwargs)

    def toSchemaAndAttrs(self, device, state):
        _, attrs = super().toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.Node
        if self.cls.daqDataType is not None:
            attrs["daqDataType"] = self.cls.daqDataType.value
        if self.cls.allowedActions is not None:
            attrs["allowedActions"] = set(self.cls.allowedActions)
        if self.cls.displayType is not None:
            attrs["displayType"] = self.cls.displayType
        if self.cls.classId is not None:
            attrs["classId"] = self.cls.classId
        return self.cls.getClassSchema(device, state).hash, attrs

    def _initialize(self, instance, value):
        node = self.cls(value)
        instance.setValue(self, node)
        ret = node._initializers
        del node._initializers
        return ret

    def __set__(self, instance, value):
        if not isinstance(value, self.cls):
            raise RuntimeError('node "{}" must be of type "{}" not "{}"'
                               .format(self.key, self.cls.__name__,
                                       type(value).__name__))
        instance.setValue(self, value)

    def _setter(self, instance, value):
        props = ((getattr(self.cls, k), v) for k, v in value.items())
        parent = getattr(instance, self.key)
        return sum((t.checkedSet(parent, v) for t, v in props), [])

    def toDataAndAttrs(self, instance):
        r = Hash()
        for k in self.cls._allattrs:
            a = getattr(instance, k, None)
            if isSet(a):
                desc = getattr(self.cls, k)
                if isinstance(desc, Slot):
                    continue
                value, attrs = desc.toDataAndAttrs(a)
                r.setElement(k, value, attrs)
        return r, {}

    def allDescriptors(self, prefix=""):
        yield from super().allDescriptors(prefix)
        for key in self.cls._allattrs:
            yield from getattr(self.cls, key).allDescriptors(
                f"{prefix}{self.key}.")


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
                r.setElement(k, value, attrs)
        return Hash(t.__name__, r), {}


class ListOfNodes(Node):
    defaultValue = Attribute()

    def toSchemaAndAttrs(self, device, state):
        h = Hash()
        for k, v in self.cls._subclasses.items():
            h[k] = v.getClassSchema(device, state).hash
            h[k, "nodeType"] = NodeType.Node

        _, attrs = super().toSchemaAndAttrs(device, state)
        attrs["nodeType"] = NodeType.ListOfNodes
        return h, attrs

    def _initialize(self, instance, value):
        if isinstance(value, Hash):
            new = [self.cls._subclasses[k](v) for k, v in value.items()]
        else:
            new = []
            for vv in value:
                if isinstance(vv, Hash):
                    # If it is a Hash, we have a single key which is the name
                    # of the class and the value for that key is the config
                    klassname = next(iter(vv))
                    new.append(self.cls._subclasses[klassname](vv[klassname]))
                # We might get strings as defaults or values
                elif isinstance(vv, str):
                    new.append(self.cls._subclasses[vv](Hash()))
        old = instance.__dict__.get(self.key, [])
        for obj in old:
            obj._parents.pop(self, None)
        ret = []
        for obj in new:
            obj._parents[self] = self.key
            ret += obj._initializers
            del obj._initializers
        instance.__dict__[self.key] = new
        return ret

    def toDataAndAttrs(self, instance):
        ret = HashList()
        for v in instance:
            r = Hash()
            t = type(v)
            for k in t._allattrs:
                value, attrs = getattr(t, k).toDataAndAttrs(getattr(v, k))
                r.setElement(k, value, attrs)
            ret.append(Hash(t.__name__, r))
        return ret, {}
