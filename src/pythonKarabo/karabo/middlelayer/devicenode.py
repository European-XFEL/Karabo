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

from karabo.native import (
    AccessMode, Assignment, Attribute, KaraboError, String, StringValue,
    get_timestamp, isSet)

from .device_client import getDevice


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

    async def finalize_init(self, instance):
        """Sets a real proxy to the MetaProxy"""
        meta_proxy = instance._getValue(self.key)
        assert isinstance(meta_proxy, MetaProxy)
        assert meta_proxy.proxy is None, "proxy already initialized"

        value = meta_proxy.deviceId
        root = instance.get_root()
        proxy = await root._sigslot.exitStack.enter_async_context(
            getDevice(value))
        proxy.__meta_deviceId = value
        meta_proxy.proxy = proxy

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
