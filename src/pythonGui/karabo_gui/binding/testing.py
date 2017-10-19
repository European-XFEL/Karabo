from contextlib import contextmanager

from karabo.common.api import DeviceStatus
from .builder import build_binding
from .proxy import DeviceProxy, PropertyProxy


@contextmanager
def assert_trait_change(obj, name):
    """A context manager which watches for traits events
    """
    events = []

    def handler():
        events.append(None)

    obj.on_trait_change(handler, name)
    try:
        yield
    finally:
        obj.on_trait_change(handler, name, remove=True)
        if len(events) == 0:
            msg = 'Expected change for trait "{}" was not observed!'
            raise AssertionError(msg.format(name))


def get_property_proxy(schema, name, device_id='TestDevice'):
    """Given a device schema and a property name, return a complete
    PropertyProxy object.
    """
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding, device_id=device_id)
    root_proxy.status = DeviceStatus.SCHEMA
    return PropertyProxy(root_proxy=root_proxy, path=name)
