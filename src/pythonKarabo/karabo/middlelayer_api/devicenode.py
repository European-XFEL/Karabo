import warnings

from karabo.native import (
    AccessMode, Assignment, Attribute, KaraboError, String, StringValue,
    get_timestamp, isSet)

from .device_client import getDevice, updateDevice


class MetaProxy:
    def __init__(self, deviceId):
        self.deviceId = StringValue(
            deviceId, timestamp=get_timestamp())
        self.proxy = None

    @property
    def value(self):
        return self.proxy or self.deviceId


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
    """
    accessMode = Attribute(AccessMode.INITONLY, dtype=AccessMode)
    assignment = Attribute(Assignment.MANDATORY, dtype=Assignment)
    displayType = Attribute("deviceNode", dtype=str)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        warnings.warn(
            "A `DeviceNode` has known issues and its usage is therefore "
            "discouraged. Please look at the documentation for help and "
            "consider using a `connectDevice` based solution in the "
            "future.",
            UserWarning, stacklevel=2)

    def toDataAndAttrs(self, value):
        if not isinstance(value, str):
            # If we have a proxy connection, the value is the `Proxy`,
            # otherwise we have a `StringValue`
            value = value.__meta_deviceId
        data, attrs = super().toDataAndAttrs(value)
        return data, attrs

    def checkedInit(self, instance, value=None):
        """Device Nodes cannot have an empty string
        """
        if not value:
            if self.defaultValue:
                return self._initialize(instance, self.defaultValue)
            raise KaraboError('Assignment is mandatory for "{}"'.format(
                self.key))
        return self._initialize(instance, value)

    async def finalize_init(self, klass):
        """Sets a real proxy to the MetaProxy"""
        meta_proxy = klass._getValue(self.key)
        assert isinstance(meta_proxy, MetaProxy)
        assert meta_proxy.proxy is None, "proxy already initialized"

        value = meta_proxy.deviceId
        proxy = await getDevice(value)
        proxy.__meta_deviceId = meta_proxy.deviceId
        meta_proxy.proxy = proxy
        klass._ss.exitStack.enter_context((await updateDevice(proxy)))

    def _initialize(self, instance, value):
        # This should not happen as we are mandatory, but we never know
        if not isSet(value):
            instance.__dict__[self.key] = None
            return []
        meta_proxy = MetaProxy(value)
        instance.__dict__[self.key] = meta_proxy
        # return an half instantiated deviceNode
        return []

    def __get__(self, instance, owner):
        if instance is None:
            return self

        value = instance._getValue(self.key)
        if isinstance(value, MetaProxy):
            value = value.value
        return value
