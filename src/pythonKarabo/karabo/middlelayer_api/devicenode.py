from asyncio import async, coroutine, Future, Queue
from itertools import chain

from .device_client import getDevice
from .enums import AccessMode, NodeType
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
    """
    def __init__(self, properties=(), commands=(), **kwargs):
        super().__init__(**kwargs)
        self.properties = properties
        self.commands = commands

    def toDataAndAttrs(self, proxy):
        if self.properties:
            return proxy._current, {}
        else:
            return Hash(), {}

    def _copy_properties(self, data, swapped):
        """return a Hash that contains our properties in Hash data"""
        ret = Hash()
        for prop in self.properties:
            if not isinstance(prop, dict):
                prop = {prop: prop}
            for name, rename in prop.items():
                if swapped:
                    name, rename = rename, name
                val = data.get(rename)
                if val is not None:
                    ret[name] = val
        return ret

    def _setter(self, instance, value):
        proxy = instance.__dict__[self.key]
        proxy._current.merge(value)

        @coroutine
        def setter():
            config = self._copy_properties(value, False)
            yield from instance.call(proxy._deviceId,
                                     "slotReconfigure", config)
        return [setter]

    @coroutine
    def initialize(self, instance, value):
        proxy = yield from getDevice(value)
        proxy._datahash = Hash()
        instance.__dict__[self.key] = proxy

        def register(theirname, myname):
            @coslot
            def slot():
                yield from instance._ss.request(proxy._deviceId, theirname)
            instance._ss.register_slot("{}.{}".format(self.key, myname), slot)

        for cmd in self.commands:
            if isinstance(cmd, dict):
                for theirname, myname in cmd.items():
                    register(theirname, myname)
            else:
                register(cmd, cmd)

        @coroutine
        def copy_data_from_proxy():
            with proxy:
                if self.properties:
                    queue = Queue()
                    proxy._queues[None].add(queue)
                    proxy._current = Hash()
                    yield from proxy
                    while True:
                        out = self._copy_properties(
                            (yield from queue.get()), True)
                        proxy._current.merge(out)
                        instance.signalChanged(Hash(self.key, out),
                                               instance.deviceId)
                else:
                    yield from Future()  # wait until we are cancelled
        async(copy_data_from_proxy())

    def toSchemaAndAttrs(self, device, state):
        h, attrs = super().toSchemaAndAttrs(device, state)
        if device is None:
            attrs["accessMode"] = AccessMode.INITONLY
            return h, attrs
        attrs["nodeType"] = NodeType.Node

        proxy = getattr(device, self.key, None)
        h = Hash()
        for prop in chain(self.properties, self.commands):
            if not isinstance(prop, dict):
                prop = {prop: prop}
            for name, rename in prop.items():
                h[rename] = proxy._schema_hash[name]
                h[rename, ...] = proxy._schema_hash[name, ...]
        return h, attrs
