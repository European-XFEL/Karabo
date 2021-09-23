from asyncio import TimeoutError, wait_for

from karabo.native import AccessMode, Assignment, KaraboError, String, isSet

from .device_client import getDevice, updateDevice


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

    A timeout in seconds can be specified via::

        class Stage(Device):
            motor = DeviceNode(timeout=2.5)

    The DeviceNode will try to connect to the device within this time frame.
    If the connection could not be established within the time interval,
    an error is raised. The default timeout is `None` and the DeviceNode will
    wait until the proxy connectiion has been established!
    """

    def __init__(self, timeout=None, **kwargs):
        super().__init__(**kwargs)
        self.timeout = timeout

    def toDataAndAttrs(self, proxy):
        deviceId = proxy.deviceId
        data, attrs = super().toDataAndAttrs(deviceId)

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

            instance.__dict__[self.key] = proxy
            instance._ss.exitStack.enter_context((await updateDevice(proxy)))

        return [inner()]

    def toSchemaAndAttrs(self, device, state):
        h, attrs = super().toSchemaAndAttrs(device, state)
        attrs["accessMode"] = AccessMode.INITONLY.value
        attrs["assignment"] = Assignment.MANDATORY.value
        attrs["displayType"] = "deviceNode"

        return h, attrs
