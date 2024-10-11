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
from asyncio import gather, get_event_loop
from itertools import chain

from karabo.native import Configurable, Descriptor, Overwrite


class InjectMixin(Configurable):
    """This is a mixin class for all classes that want to inject parameters

    A parameter injection is a modification of the class of an object. Since
    we do not want to modify the classes of all instances, we generate a
    fresh class for every object, which inherits from our class. This new
    class is completely empty, so we can modify it at will. Once we have done
    that, await
    :meth:`~karabo.middlelayer.InjectMiXin.publishInjectedParameters`::

        class MyDevice(Device):

            async def onInitialization(self):
                # should it be needed to test that the node is there
                self.my_node = None

            async def inject_something(self):
                # inject a new property into our personal class:
                self.__class__.injected_string = String()
                self.__class__.my_node = Node(MyNode, displayedName="position")
                await self.publishInjectedParameters()

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

    _needs_notify = False

    def __new__(cls, configuration={}, **kwargs):
        """each object gets its own personal class, that it may modify"""
        newtype = type(cls.__name__, (cls,), {})
        ret = super(Configurable, cls).__new__(newtype)
        # Make the original module available
        ret.__module_orig__ = cls.__module__
        return ret

    async def _run(self, **kwargs):
        await super()._run(**kwargs)

    def _collect_attrs(self):
        cls = self.__class__
        added_attrs = []
        for k, v in cls.__dict__.items():
            if isinstance(v, Descriptor):
                if v.key == '(unknown key)':
                    v.key = k
                    added_attrs.append(k)
            elif isinstance(v, Overwrite):
                added_attrs.append(k)
                setattr(cls, k, v.overwrite(getattr(super(cls, cls), k)))

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
        return added_attrs

    async def publishInjectedParameters(self, *args, **kwargs):
        """Publish all changes in the parameters of this object

        This is also the time when the injected parameters get initialized.
        As keyword arguments, you may give the initial values for the
        injected parameters::

            self.__class__.some_number = Int32()
            await self.publishInjectedParameters(some_number=3)

        Parameter injection as arguments in pairs is possible as well::

            self.__class__.some_number = Int32()
            await self.publishInjectedParameters("some_number", 3)

        Note: Arguments will overwrite eventual keyword arguments.
        """
        if len(args) % 2 > 0:
            raise RuntimeError("Arguments passed need to be pairs!")

        kwargs.update(zip(args[::2], args[1::2]))

        initializers = []
        for k in self._collect_attrs():
            t = getattr(type(self), k)
            init = t.checkedInit(self, kwargs.get(k))
            initializers.extend(init)

        await gather(*initializers)

        self._needs_notify = True
        get_event_loop().call_soon(self._notifyNewInjection)

    def _notifyNewInjection(self):
        if self._needs_notify:
            self._notifyNewSchema()
            self.signalChanged(self.configurationAsHash(), self.deviceId)
            self._needs_notify = False
