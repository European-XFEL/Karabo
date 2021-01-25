from asyncio import (
    CancelledError, ensure_future, TimeoutError, wait_for, Queue)
from itertools import chain

from karabo.native import isSet, KaraboError, String
from karabo.native import AccessMode, Assignment, NodeType, Hash

from .device_client import getDevice, updateDevice
from .signalslot import coslot


class DeviceNode(String):
    """Copy a remote device into the local's namespace

    A device that controls another device may add a device node such that
    it has a proxy to that device while both are running. The controlled
    device may be configured at initialization time.

    As an example, a stage controlling some motor::

        class Stage(Device):
            motor = DeviceNode()

    Then the motor can be acessed under ``self.motor``. Before
    instantiation, a device node just looks like a string, where the
    id of the target device can be entered.

    Sometimes it is advisable that some of the properties of the target
    device are also made available to outside users. To this end, a list of
    properties and commands to be copied can be given::

        class Stage(Device):
            motor = DeviceNode(properties=["position", "speed"],
                               commands=["start"])

    A timeout in seconds can be specified via::

        class Stage(Device):
            motor = DeviceNode(timeout=1.5)

    The DeviceNode will try to connect to the device within this time frame. If
    the connection could not be established, an error is raised.
    """

    def __init__(self, properties=(), commands=(), timeout=None, **kwargs):
        super().__init__(**kwargs)

        self.properties = properties
        self.commands = commands
        self.timeout = timeout
        if self.properties or self.commands:
            for default in ("deviceId", "state", "alarmCondition"):
                if default not in self.properties:
                    self.properties.append(default)

    def toDataAndAttrs(self, proxy):
        if self.properties or self.commands:
            return proxy._current, {}
        else:
            deviceId = proxy.deviceId
            data, attrs = super().toDataAndAttrs(deviceId)

            return data, attrs

    def _copy_properties(self, data):
        """return a Hash that contains our properties in Hash data"""
        ret = Hash()
        for key in self.properties:
            val = data.get(key)
            if val is not None:
                attrs = data[key, ...]
                ret[key] = val
                ret[key, ...].update(attrs)

        return ret

    def _setter(self, instance, data):
        proxy = instance.__dict__[self.key]

        async def setter():
            await instance.call(proxy._deviceId,
                                "slotReconfigure", data)

        return [setter]

    async def _main_loop(self, proxy, instance):
        """relay data coming from *proxy* on behalf of *instance*"""
        try:
            queue = Queue()
            proxy._queues[None].add(queue)
            proxy._current = Hash()
            with proxy:
                await updateDevice(proxy)
                while True:
                    data = await queue.get()
                    out = self._copy_properties(data)
                    if out.empty():
                        continue
                    proxy._current.merge(out)
                    instance.signalChanged(Hash(self.key, out),
                                           instance.deviceId)
        except CancelledError:
            raise
        except Exception:
            instance.logger.exception(
                'device node "{}" failed'.format(self.key))

    def checkedInit(self, instance, value=None):
        """Device Nodes cannot have an empty string
        """
        if not value:
            if self.defaultValue:
                return self._initialize(instance, self.defaultValue)
            raise KaraboError('Assignment is mandatory for "{}"'.format(
                self.key))
        return self._initialize(instance, value)

    def _initialize(self, instance, value):
        # This should not happen as we are mandatory, but we never know
        if not isSet(value):
            instance.__dict__[self.key] = None
            return []

        async def inner():
            try:
                proxy = await wait_for(getDevice(value),
                                       timeout=self.timeout)
            except TimeoutError:
                # We can accept a connection attempt only for a limited time,
                # after that, the device will go offline
                raise KaraboError(
                    'The DeviceNode with key "{}" timed out and could not '
                    'establish a proxy to "{}"'.format(self.key, value))

            proxy._current = Hash()
            instance.__dict__[self.key] = proxy
            if self.commands or self.properties:
                instance._notifyNewSchema()

            def register(command):
                @coslot
                async def slot():
                    await instance._ss.request(proxy._deviceId, command)

                instance._ss.register_slot(
                    "{}.{}".format(self.key, command), slot)

            for command in self.commands:
                register(command)

            instance._ss.exitStack.enter_context((await updateDevice(proxy)))
            if self.properties:
                ensure_future(self._main_loop(proxy, instance))

        return [inner()]

    def toSchemaAndAttrs(self, device, state):
        h, attrs = super().toSchemaAndAttrs(device, state)
        if device is None or (not self.properties and not self.commands):
            attrs["accessMode"] = AccessMode.INITONLY.value
            attrs["assignment"] = Assignment.MANDATORY.value
            return h, attrs
        attrs["nodeType"] = NodeType.Node
        attrs["displayType"] = "deviceNode"

        proxy = getattr(device, self.key, None)
        h = Hash()
        # test whether or not proxy is a NoneValue or None
        if isSet(proxy):
            for name in chain(self.properties, self.commands):
                h[name] = proxy._schema_hash[name]
                h[name, ...] = proxy._schema_hash[name, ...]

        return h, attrs
