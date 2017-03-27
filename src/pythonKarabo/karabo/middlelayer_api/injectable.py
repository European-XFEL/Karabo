from asyncio import coroutine, gather

from .hash import Descriptor
from .schema import Configurable, MetaConfigurable


class MetaInjectable(MetaConfigurable):
    def __init__(self, name, bases, namespace):
        self._added_attrs = []
        super().__init__(name, bases, namespace)
        self._added_attrs.clear()

    def __setattr__(self, name, value):
        super().__setattr__(name, value)
        self._added_attrs.append(name)
        if isinstance(value, Descriptor):
            value.key = name



class Injectable(Configurable):
    """This is a mixin class for all classes that want to inject parameters

    A parameter injection is a modification of the class of an object. Since
    we do not want to modify the classes of all instances, we generate a
    fresh class for every object, which inherits from our class. This new
    class is completely empty, so we can modify it at will. Once we have done
    that, call
    :meth:`~karabo.middlelayer.Injectable.publishInjectedParameters`::

        class MyDevice(Injectable, Device):
            def inject_something(self):
                # inject a new property into our personal class:
                self.__class__.injected_string = String()
                self.publishInjectedParameters()

                # use the property as any other property:
                self.injected_string = "whatever"
    """
    def __new__(cls, configuration={}):
        """each object gets its own personal class, that it may modify"""
        newtype = MetaInjectable(cls.__name__, (cls,), {})
        return super(Configurable, cls).__new__(newtype)

    @coroutine
    def publishInjectedParameters(self, **kwargs):
        """Publish all changes in the parameters of this object"""
        cls = self.__class__
        added_attrs = list(cls._added_attrs)
        cls._attrs = [attr for attr, value in cls.__dict__.items()
                      if isinstance(value, Descriptor)]
        cls._allattrs = list(super(cls, cls)._allattrs)
        seen = set(cls._allattrs)
        cls._allattrs.extend(attr for attr in cls._attrs if attr not in seen)
        self._register_slots()

        _initializers = []
        for k in added_attrs:
            t = getattr(type(self), k)
            init = t.checkedInit(self, kwargs.get(k))
            _initializers.extend(init)
        self._added_attrs = []

        yield from gather(*_initializers)

        self._notifyNewSchema()
        self.signalChanged(self.configurationAsHash(), self.deviceId)
        cls._added_attrs.clear()