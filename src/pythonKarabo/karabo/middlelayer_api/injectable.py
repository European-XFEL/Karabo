from .hash import Descriptor
from .schema import Configurable, MetaConfigurable


class MetaInjectable(MetaConfigurable):
    def __setattr__(self, name, value):
        super().__setattr__(name, value)
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
    def __new__(cls, configuration={}, parent=None, key=None):
        """each object gets its own personal class, that it may modify"""
        newtype = MetaInjectable(cls.__name__, (cls,), {})
        return super(Configurable, cls).__new__(newtype)

    def publishInjectedParameters(self):
        """Publish all changes in the parameters of this object"""
        cls = self.__class__
        cls._attrs = [attr for attr, value in cls.__dict__.items()
                      if isinstance(value, Descriptor)]
        cls._allattrs = list(super(cls, cls)._allattrs)
        seen = set(cls._allattrs)
        cls._allattrs.extend(attr for attr in cls._attrs if attr not in seen)
        self._notifyNewSchema()
        self.signalChanged(self.configurationAsHash(), self.deviceId)
