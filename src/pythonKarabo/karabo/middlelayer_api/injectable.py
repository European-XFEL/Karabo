from asyncio import coroutine, gather
from itertools import chain

from karabo.native.data.hash import Descriptor
from karabo.native.data.schema import Configurable, Overwrite, MetaConfigurable


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
        elif isinstance(value, Overwrite):
            setattr(self, name,
                    value.overwrite(getattr(super(self, self), name)))


class Injectable(Configurable):
    """This is a dummy class for backward compatiblity
    """
    def __init__(self, configuration={}):
        super(Injectable, self).__init__(configuration)


class InjectMixin(Configurable):
    """This is a mixin class for all classes that want to inject parameters

    A parameter injection is a modification of the class of an object. Since
    we do not want to modify the classes of all instances, we generate a
    fresh class for every object, which inherits from our class. This new
    class is completely empty, so we can modify it at will. Once we have done
    that, yield from
    :meth:`~karabo.middlelayer.InjectMiXin.publishInjectedParameters`::

        class MyDevice(Device):
            @coroutine
            def onInitialization(self):
                # should it be needed to test that the node is there
                self.my_node = None

            @coroutine
            def inject_something(self):
                # inject a new property into our personal class:
                self.__class__.injected_string = String()
                self.__class__.my_node = Node(MyNode, displayedName="position")
                yield from self.publishInjectedParameters()

                # use the property as any other property:
                self.injected_string = "whatever"
                # the test that the node is there is superflous here
                if self.my_node is not None:
                    self.my_node.reached = False

        class MyNode(Configurable):
            reached = Bool(displayedName="On position",
                           description="On position flag",
                           defaultValue=True,
                           accessMode=AccessMode.RECONFIGURABLE
            )

    Middlelayer class based injection differs strongly from C++ and
    bound api parameter injection, and the following points should
    be remembered:

    * classes can only be injected into the top layer of the empty class
      and, consequently, of the schema rendition
    * the order of injection defines the order in schema rendition
    * classes injected can be simple (a Float, Bool, etc.) or complex
      (a node, an entire class hierarchies, etc.)
    * later modification of injected class structure is not seen in the
      schema. Modification can only be achieved by overwriting the top level
      assignment of the class and calling `publishInjectedParameters`
    * injected classes are not affected by later calls to
      `publishInjectedParameters` used to inject other classes
    * deleted (del) injected classes are removed from the schema by calling
      `publishInjectedParameters`
    """

    def __new__(cls, configuration={}, **kwargs):
        """each object gets its own personal class, that it may modify"""
        newtype = MetaInjectable(cls.__name__, (cls,), {})
        ret = super(Configurable, cls).__new__(newtype)
        # Make the original module available
        ret.__module_orig__ = cls.__module__
        return ret

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
