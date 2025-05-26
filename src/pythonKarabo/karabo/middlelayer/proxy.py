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
import asyncio
import time
from collections import defaultdict
from contextlib import suppress
from inspect import isfunction
from weakref import WeakSet

from aiormq.exceptions import AMQPException

from karabo.common.api import WeakMethodRef
from karabo.middlelayer.synchronization import synchronize, synchronous
from karabo.native import (
    Descriptor, Hash, KaraboError, KaraboValue, NDArray, NodeType, NoneValue,
    Slot, Timestamp, Type, Weak, get_timestamp, isSet)

UNSUBSCRIBE_TIMEOUT = 5


class _ProxyBase:
    _parent = Weak()

    @classmethod
    def __dir__(cls):
        return cls._allattrs

    def __repr__(self):
        subs = ", ".join(f"{k}={getattr(self, k)!r}"
                         for k, v in self.__class__.__dict__.items()
                         if isinstance(v, Descriptor) and
                         not isinstance(v, Slot) and hasattr(self, k))
        return f"{self.__class__.__name__}[{subs}]"

    def _repr_html_generator_(self, nest=0):
        if nest == 0:
            yield "<table>"
        for attr in self._allattrs:
            val = getattr(self, attr, None)
            if isinstance(val, _ProxyBase):
                yield ('<tr><td style="padding-left:{}em"><b>{}</b></td>'
                       '<td/></tr>'.format(nest + 1, attr))
                yield from val._repr_html_generator_(nest + 1)
            elif isinstance(val, KaraboValue):
                yield ('<tr><td style="padding-left:{}em">{}</td><td>'
                       .format(nest + 1, attr))
                yield from val._repr_html_generator_()
                yield '</td></tr>'
        if nest == 0:
            yield "</table>"

    def _repr_html_(self):
        return "".join(self._repr_html_generator_())

    def _repr_pretty_(self, p, cycle):
        if cycle:
            p.text(str(self))
        else:
            for attr in self._allattrs:
                val = getattr(self, attr, None)
                if isinstance(val, (_ProxyBase, KaraboValue)):
                    p.break_()
                    with p.group(4, f"{attr}:"):
                        p.breakable()
                        p.pretty(val)

    def _getValue(self, key):
        self._parent._use()
        ret = self.__dict__.get(key)
        if ret is None:
            ret = NoneValue(descriptor=getattr(self.__class__, key))
            ret._parent = self._parent
        return ret


class ProxyBase(_ProxyBase):
    """The base for proxies to a Karabo device

    The :class:`ProxyFactory` will subclass this class and add
    the necessary descriptors.
    """

    def __init__(self):
        super().__init__()
        self._parent = self

    def _use(self):
        pass

    def _callSlot(self, descriptor):
        """this gets called once the user calls a slot"""

    def _onChanged_r(self, change, instance):
        for k, v, a in change.iterall():
            descr = getattr(type(instance), k, None)
            if descr is not None:
                if isinstance(descr, ProxyNodeBase):
                    self._onChanged_r(v, getattr(instance, descr.key))
                elif not isinstance(descr, ProxySlotBase):
                    converted = descr.toKaraboValue(v, strict=False)
                    converted.timestamp = Timestamp.fromHashAttributes(a)
                    converted._parent = self
                    instance.__dict__[descr.key] = converted
                    self._notifyChanged(descr, converted)

    def _onChanged_timestamp_r(self, change, timestamp, instance):
        """Recursively set data on a proxy with a fixed timestamp

        This function can be optionally used for pipeline data
        """
        for k, v, a in change.iterall():
            descr = getattr(type(instance), k, None)
            if descr is not None:
                if isinstance(descr, ProxyNodeBase):
                    self._onChanged_timestamp_r(
                        v, timestamp, getattr(instance, descr.key))
                elif not isinstance(descr, ProxySlotBase):
                    converted = descr.toKaraboValue(v, strict=False)
                    converted.timestamp = timestamp
                    converted._parent = self
                    instance.__dict__[descr.key] = converted
                    self._notifyChanged(descr, converted)

    def _onChanged(self, change):
        """call this when the remote device changed

        :change: is the Hash with the changes
        """
        self._onChanged_r(change, self)

    def configurationAsHash(self):
        """Extract a configuration in a Hash from the proxy

        Some values of a `ListOfNodes` appear in the configuration, but are not
        available on the proxy (for now).
        """
        ret = Hash()

        def recurse(proxy):
            nonlocal ret
            for key in proxy._allattrs:
                descr = getattr(type(proxy), key, None)
                if descr is None:
                    # Unsupported types
                    continue
                if isinstance(descr, ProxySlotBase):
                    # Slots don't appear in Configuration
                    continue
                elif isinstance(descr, ProxyNodeBase):
                    # recurse Nodes
                    recurse(getattr(proxy, descr.key))
                else:
                    value = getattr(proxy, key, None)
                    if isSet(value):
                        data, attrs = descr.toDataAndAttrs(value)
                        ret[descr.longkey] = data
                        ret[descr.longkey, ...].update(attrs)
        recurse(self)

        return ret

    def _notifyChanged(self, descriptor, value):
        """this is called by _onChanged for each change"""


class ProxyNodeBase(Descriptor):
    """The base class for all nodes in a Proxy

    This is the :class:`Descriptor` for a node in the class. The actual
    data's base class is :class:`SubProxyBase`.
    """

    def __init__(self, *, cls, **kwargs):
        super().__init__(**kwargs)
        self.cls = cls

    def __get__(self, instance, owner):
        if instance is None:
            return self
        ret = instance.__dict__.get(self.key, None)
        if ret is not None:
            return ret
        sub = self.cls()
        if isinstance(instance, SubProxyBase):
            sub._parent = instance._parent
        else:
            sub._parent = instance
        instance.__dict__[self.key] = sub
        return sub


class ProxySlotBase(Slot, Descriptor):
    """The base class for Slots in proxies"""

    def __get__(self, instance, owner):
        if instance is None:
            return self

        def method(myself, *args, **kwargs):
            return myself._parent._callSlot(self, *args, **kwargs)
        method.__doc__ = self.description
        return method.__get__(instance, owner)


class SubProxyBase(_ProxyBase):
    """The base class for nodes in a Proxy"""

    def setValue(self, desc, value):
        return self._parent.setValue(desc, value)


class ProxyFactory:
    """Create a proxy for Karabo devices

    This is an customizable factory class to create proxies for Karabo
    devices. You can customize which classes are used in the proxy
    building process by setting the following class attributes:

    Proxy
        This is the base class for all proxies. It should inherit from
        :class:`ProxyBase`.

    ProxySlot
        This is the base class for all slots of a device. It should inherit
        from :class:`ProxySlotBase`.

    ProxyNode
        The base class for all descriptors of nodes. Should inherit
        from :class:`ProxyNodeBase`.

    SubProxy
        This is the base class for all the node values in a proxy.
        It should inherit from :class:`SubProxyBase`.
    """
    Proxy = ProxyBase
    ProxyNode = ProxyNodeBase
    SubProxy = SubProxyBase

    node_factories = dict(
        Slot=ProxySlotBase,
        NDArray=NDArray)

    @classmethod
    def createNode(cls, key, node, prefix, **kwargs):
        sub = cls.createNamespace(node, f"{prefix}{key}.")
        Cls = type(key, (cls.SubProxy,), sub)
        return cls.ProxyNode(key=key, cls=Cls, **kwargs)

    @classmethod
    def register_special(cls, name, special):
        """register a special class *special* with *name*

        This adds a new handler for a classId. Note that this is registered
        in the class of the caller, and normal Python inheritance rules apply,
        so you can overload the registration in a specialized class.
        """
        cls.node_factories[name] = special

    @classmethod
    def createNamespace(cls, schema, prefix=""):
        namespace = {}
        for k, v, a in schema.iterall():
            nodeType = NodeType(a["nodeType"])
            if nodeType is NodeType.Leaf:
                descriptor = Type.fromname[a["valueType"]](
                    strict=False, key=k, **a)
                descriptor.longkey = prefix + k
            elif nodeType is NodeType.Node:
                classId = a.get("classId")
                factory = cls.node_factories.get(classId, cls.createNode)
                descriptor = factory(key=k, node=v, prefix=prefix,
                                     strict=False, **a)
            else:
                continue  # currently unsupported NodeType
            descriptor.longkey = prefix + k
            namespace[k] = descriptor
        namespace["_allattrs"] = list(schema)
        return namespace

    @classmethod
    def createProxy(cls, schema):
        namespace = cls.createNamespace(schema.hash)
        namespace["_schema_hash"] = schema.hash
        return type(schema.name, (cls.Proxy,), namespace)

    @classmethod
    def updateSchema(cls, proxy, schema):
        """update the schema of proxy"""
        def recurse(newcls, instance):
            instance.__class__ = newcls
            for key in dir(newcls):
                descriptor = getattr(newcls, key)
                if isinstance(descriptor, ProxyNodeBase):
                    value = getattr(instance, key, None)
                    if isinstance(value, SubProxyBase):
                        recurse(descriptor.cls, value)
                    else:
                        # something became a Node which wasn't one before.
                        # simply delete it to start anew at the next change
                        instance.__dict__.pop(key, None)
        newcls = cls.createProxy(schema)
        recurse(newcls, proxy)


class DeviceClientProxyFactory(ProxyFactory):
    class Proxy(ProxyBase):
        """A proxy for a remote device

        This proxy represents a real device somewhere in the karabo
        system. It is typically created by :func:`getDevice`, do not
        create it yourself.

        Properties and slots may be accessed as if one would access
        the actual device object, as in::

            device = getDevice("someDevice")
            device.speed = 7
            device.start()

        Note that in order to reduce network traffic, the proxy device
        is initially not *connected*, meaning that it does not receive
        updates of properties from the device. There are two ways to
        change that: you can call :func:`updateDevice` to get the
        current configuration once, or you may use the proxy in a with
        statment, which will connect the device during the execution
        of that statement, as in::

            with getDevice("someDevice") as device:
                print(device.speed)
        """

        def __init__(self, device, deviceId, sync):
            super().__init__()
            # device holding the proxy, e.g. DeviceClient (CLI) or
            # a middlelayer device
            self._device = device
            # None queue is global
            self._queues = defaultdict(WeakSet)
            # the deviceId
            self._deviceId = deviceId
            # variable used for autodisconnect
            self._used = 0
            # Hash container used for bulksetting
            self._sethash = Hash()
            # indicates if we are in sync with the eventloop
            self._sync = sync
            # our indicator if the underlying device is alive
            self._alive = True
            self._running_tasks = set()
            self._last_update_task = None
            self._schemaUpdateConnected = False
            self._lock_count = 0
            self._remote_output_channel = WeakSet()
            self._configurationHandler = None

        def _notifyChanged(self, descriptor, value):
            for q in self._queues[descriptor.longkey]:
                q.put_nowait(value)

        def _onChanged(self, change):
            if self._configurationHandler is not None:
                self._configurationHandler(change)
            super()._onChanged(change)
            for q in self._queues[None]:
                q.put_nowait(change)

        def setConfigHandler(self, changeHandler):
            """subscribe a configuration Change Handler

            This feature is supposed to be used for test and is experimental.
            Any feature built on it could break.
            Pass a synchronous function to it. To unsubscribe from it,
            pass a ``None``

            :changeHandler: a function taking a hash in input or a None
            """
            assert changeHandler is None or isfunction(changeHandler)
            self._configurationHandler = changeHandler

        def setValue(self, desc, value):
            """Set a value belonging to a descriptor on a proxy
            """
            self._use()
            loop = asyncio.get_event_loop()
            assert isinstance(value, KaraboValue)
            if loop.sync_set:
                # we are in a thread not running on Eventloop and
                # immediately send out our changes
                h = Hash()
                h[desc.longkey], _ = desc.toDataAndAttrs(value)
                loop.sync(self._raise_on_death(
                    self._device.call, self._deviceId, "slotReconfigure", h),
                    timeout=-1, wait=True)
            elif (self._last_update_task is not None and
                  self._last_update_task.done() and
                  self._last_update_task.exception() is not None):
                task = self._last_update_task
                self._last_update_task = None
                task.result()  # raise the exception
            else:
                # we are on the Eventloop and can do bulksetting
                update = not self._sethash
                self._sethash[desc.longkey], _ = desc.toDataAndAttrs(value)
                if update:
                    self._last_update_task = asyncio.ensure_future(
                        self._update(self._last_update_task))

        def _callSlot(self, descriptor, timeout=-1, wait=True):
            async def inner():
                await self._update()
                self._device._use()
                return (await self._device.call(self._deviceId,
                                                descriptor.longkey))

            return asyncio.get_event_loop().sync(
                self._raise_on_death(inner), timeout, wait)

        async def _update(self, task=False):
            """assure all properties are properly set

            property settings are cached in self._sethash. This method
            will assure they have properly been sent to the device, or it
            will send it itself. An error is raised if that fails.

            :param task: is an already running update task, if supplied.
            self._last_update_task is checked otherwise.
            """
            if task is False:
                task = self._last_update_task
                self._last_update_task = None
            if task is not None:
                # wait until task is finished
                await task
            if self._sethash:
                # reconfigure the proxy and empty the bulk hash
                sethash, self._sethash = self._sethash, Hash()
                await self._device._sigslot.request(
                    self._deviceId, "slotReconfigure", sethash)

        def _notify_gone(self):
            """The underlying device has gone"""
            self._alive = False
            for task in self._running_tasks:
                task.cancel()
            # actively set the state to UNKNOWN
            h = Hash("state", "UNKNOWN")
            timestamp = get_timestamp()
            timestamp.toHashAttributes(h)
            self._onChanged(h)

        async def _notify_new(self):
            """The underlying device comes back online"""
            schema, _ = await self._device.call(
                self._deviceId, "slotGetSchema", False)
            DeviceClientProxyFactory.updateSchema(self, schema)
            # get configuration
            await self.update_proxy()
            if not self._alive:
                self._alive = True

        async def _raise_on_death(self, func, *args):
            """exec function *func* but raise KaraboError if proxy is orphaned

            This coroutine creates a coroutine from *func*. If the
            device connected to this proxy dies while the coroutine is
            executed, a KaraboError is raised.
            """
            if not self._alive:
                raise KaraboError(
                    f'device "{self._deviceId}" died')
            coro = func(*args)
            task = asyncio.ensure_future(coro)
            self._running_tasks.add(task)
            task.add_done_callback(
                lambda fut: self._running_tasks.discard(fut))
            try:
                return (await task)
            except asyncio.CancelledError:
                if self._alive:
                    raise
                else:
                    raise KaraboError(
                        f'device "{self._deviceId}" died')

        async def initialize_proxy(self):
            """Initialize the proxy by subscribing to updates

            Note: This increments the use counter and will decrease the
            counter after a period to make sure the proxy is connected
            on creation.
            """
            self.__enter__()
            loop = asyncio.get_event_loop()
            loop.call_later(UNSUBSCRIBE_TIMEOUT,
                            WeakMethodRef(self.__exit__), None, None, None)

        def __enter__(self):
            self._used += 1
            if self._used == 1:
                self._device._sigslot.connect(self._deviceId, "signalChanged",
                                              self._device.slotChanged)
            return self

        def __exit__(self, exc_type, exc_value, traceback):
            """Exit the proxy by disconnecting from the signals
            """
            if self._used > 0:
                # Make sure to protect against multiple disconnect calls
                self._used -= 1
                if self._used == 0:
                    self._device._sigslot.disconnect(self._deviceId,
                                                     "signalChanged",
                                                     self._device.slotChanged)

        async def __aenter__(self):
            self._used += 1
            if self._used == 1:
                await self._device._sigslot.async_connect(
                    self._deviceId, "signalChanged", self._device.slotChanged)
            return self

        async def __aexit__(self, exc_type, exc_value, traceback):
            """Exit the proxy by disconnecting from the signals
            """
            if self._used > 0:
                # Make sure to protect against multiple disconnect calls
                self._used -= 1
                if self._used == 0:
                    await self._device._sigslot.async_disconnect(
                        self._deviceId, "signalChanged",
                        self._device.slotChanged)

        def _disconnectSchemaUpdated(self):
            if self._schemaUpdateConnected:
                self._device._sigslot.disconnect(
                    self._deviceId, "signalSchemaUpdated",
                    self._device.slotSchemaUpdated)
            self._schemaUpdateConnected = False

        async def _async_disconnectSchemaUpdated(self):
            if self._schemaUpdateConnected:
                await self._device._sigslot.async_disconnect(
                    self._deviceId, "signalSchemaUpdated",
                    self._device.slotSchemaUpdated)
            self._schemaUpdateConnected = False

        def _disconnect_outputs(self):
            """Disconnect all remote output channels"""
            # Note: In rare cases, e.g. command line, the broker connection
            # is already gone when we are collected, hence we do not throw
            # exceptions from here!
            outputs = list(self._remote_output_channel)
            for channel in outputs:
                channel.disconnect()

        def __del__(self):
            self._disconnect_outputs()
            with suppress(AMQPException):
                self._disconnectSchemaUpdated()
            if self._used > 0:
                self._used = 1
                with suppress(AMQPException):
                    self.__exit__(None, None, None)

        async def delete_proxy(self):
            """Delete the proxy, disconnect all channels and signals"""
            self._disconnect_outputs()
            with suppress(AMQPException):
                await self._async_disconnectSchemaUpdated()
            if self._used > 0:
                self._used = 1
                with suppress(AMQPException):
                    await self.__aexit__(None, None, None)

        async def update_proxy(self):
            """Send out cached changes and get current configuration
            """
            await self._update()
            conf, _ = await self._device.call(self._deviceId,
                                              "slotGetConfiguration")
            self._onChanged(conf)
            return self


class AutoDisconnectProxyFactory(DeviceClientProxyFactory):
    class Proxy(DeviceClientProxyFactory.Proxy):
        def __init__(self, device, deviceId, sync):
            super().__init__(device, deviceId, sync)
            self._interval = 1
            self._lastused = time.time()
            self._task = asyncio.ensure_future(self._connector())

        @synchronous
        def _use(self):
            self._lastused = time.time()
            if self._task.done():
                self._connect()

        @synchronize
        async def _connect(self):
            if self._task.done():
                await self.update_proxy()
                self._task = asyncio.ensure_future(self._connector())

        async def _connector(self):
            with self:
                while True:
                    delta = self._interval - time.time() + self._lastused
                    if delta < 0:
                        return
                    await asyncio.sleep(delta)
