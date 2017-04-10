from asyncio import coroutine, gather
from itertools import chain

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
    that, yield from
    :meth:`~karabo.middlelayer.Injectable.publishInjectedParameters`::

        class MyDevice(Injectable, Device):
            @coroutine
            def inject_something(self):
                # inject a new property into our personal class:
                self.__class__.injected_string = String()
                yield from self.publishInjectedParameters()

                # use the property as any other property:
                self.injected_string = "whatever"
    """

    def __new__(cls, configuration={}):
        """each object gets its own personal class, that it may modify"""
        newtype = MetaInjectable(cls.__name__, (cls,), {})
        return super(Configurable, cls).__new__(newtype)

    @coroutine
    def _run(self, **kwargs):
        yield from super()._run(**kwargs)

    def _collect_attrs(self):
        cls = self.__class__
        added_attrs = list(cls._added_attrs)

        def unique_everseen(iter):
            seen = set()
            for i in iter:
                if i not in seen:
                    seen.add(i)
                    yield i

        cls._attrs = list(unique_everseen(
            attr for attr in chain(cls._attrs, added_attrs)
            if attr in cls.__dict__))

        cls._allattrs = list(super(cls, cls)._allattrs)
        seen = set(cls._allattrs)
        cls._allattrs.extend(attr for attr in cls._attrs if attr not in seen)
        self._register_slots()
        self._added_attrs.clear()
        return added_attrs

    @coroutine
    def publishInjectedParameters(self, **kwargs):
        """Publish all changes in the parameters of this object

        This is also the time when the injected parameters get initialized.
        As keyword arguments, you may give the initial values for the
        injected parameters::

            self.__class__.some_number = Int32()
            yield from self.publishInjectedParameters(some_number=3)
        """
        initializers = []
        for k in self._collect_attrs():
            t = getattr(type(self), k)
            init = t.checkedInit(self, kwargs.get(k))
            initializers.extend(init)

        yield from gather(*initializers)

        self._notifyNewSchema()
        self.signalChanged(self.configurationAsHash(), self.deviceId)
