from asyncio import (
    async, CancelledError, coroutine, TimeoutError, wait_for, Queue)
from collections import OrderedDict
from itertools import chain

from .basetypes import isSet
from .device_client import getDevice, lock
from .enums import AccessMode, Assignment, NodeType
from .exceptions import KaraboError
from .signalslot import coslot
from .hash import Hash, String


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
            motor = DeviceNode(properties=["position", {"speed": "velocity"}],
                               commands=["start"])

    Note how the property ``speed`` is renamed to ``velocity`` on the way.

    If the other device should be locked by this device, set the attribute
    ``lock=True``.

    A timeout in seconds can be specified via

        class Stage(Device):
            motor = DeviceNode(timeout=1.5)

    The device with try to connect to the device within this time frame. If
    the connection could not be established, an error is raised.
    """

    def __init__(self, properties=(), commands=(), timeout=None,
                 lock=False, **kwargs):
        super().__init__(**kwargs)

        def recode(names):
            ret = OrderedDict()
            for name in names:
                if isinstance(name, dict):
                    assert all(isinstance(k, str) and isinstance(v, str)
                               for k, v in name.items())
                    ret.update(name)
                else:
                    assert isinstance(name, str)
                    ret[name] = name
            return ret

        self.properties = recode(properties)
        self.commands = recode(commands)
        self.timeout = timeout
        self.lock = lock
        if self.properties or self.commands:
            for default in ("deviceId", "state", "alarmCondition"):
                if default not in self.properties:
                    self.properties[default] = default

    def toDataAndAttrs(self, proxy):
        if self.properties or self.commands:
            return proxy._current, {}
        else:
            deviceId = proxy.deviceId
            data, attrs = super().toDataAndAttrs(deviceId)

            return data, attrs

    def _copy_properties(self, data, swapped):
        """return a Hash that contains our properties in Hash data"""
        ret = Hash()
        for name, rename in self.properties.items():
            if swapped:
                name, rename = rename, name
            val = data.get(rename)
            if val is not None:
                attrs = data[rename, ...]
                ret[name] = val
                ret[name, ...].update(attrs)

        return ret

    def _setter(self, instance, value):
        proxy = instance.__dict__[self.key]

        @coroutine
        def setter():
            config = self._copy_properties(value, False)
            yield from instance.call(proxy._deviceId,
                                     "slotReconfigure", config)

        return [setter]

    @coroutine
    def _main_loop(self, proxy, instance):
        """relay data coming from *proxy* on behalf of *instance*"""
        try:
            queue = Queue()
            proxy._queues[None].add(queue)
            proxy._current = Hash()
            with proxy:
                yield from proxy
                while True:
                    data = yield from queue.get()
                    out = self._copy_properties(data, True)
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

    @coroutine
    def initialize(self, instance, value):
        # This should not happen as we are mandatory, but we never know
        if not isSet(value):
            instance.__dict__[self.key] = None
            return
        try:
            proxy = yield from wait_for(getDevice(value),
                                        timeout=self.timeout)
        except TimeoutError:
            # We can accept a connection attempt only for a limited time, after
            # that, the device will go offline
            raise KaraboError(
                'The DeviceNode with key "{}" timed out and could not '
                'establish a proxy to "{}"'.format(self.key, value))

        proxy._current = Hash()
        instance.__dict__[self.key] = proxy
        instance._notifyNewSchema()

        def register(theirname, myname):
            @coslot
            def slot():
                yield from instance._ss.request(proxy._deviceId, theirname)

            instance._ss.register_slot("{}.{}".format(self.key, myname), slot)

        for theirname, myname in self.commands.items():
            register(theirname, myname)

        instance._ss.exitStack.enter_context((yield from proxy))
        if self.lock:
            instance._ss.exitStack.enter_context((yield from lock(proxy)))
        if self.properties:
            async(self._main_loop(proxy, instance))

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
            for name, rename in chain(self.properties.items(),
                                      self.commands.items()):
                h[rename] = proxy._schema_hash[name]
                h[rename, ...] = proxy._schema_hash[name, ...]
        return h, attrs
