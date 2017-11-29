import contextlib
import sys
import unittest

from PyQt4.QtCore import QEventLoop
from PyQt4.QtGui import QApplication

from karabo.common.api import Capabilities, DeviceStatus
from karabo.middlelayer import AccessLevel, Hash
from karabogui.binding.api import (
    build_binding, DeviceClassProxy, DeviceProxy, PropertyProxy)
import karabogui.singletons.api as singletons_mod


class GuiTestCase(unittest.TestCase):
    """ A convenient base class for gui test cases
    """

    def setUp(self):
        app = QApplication.instance()
        if app is None:
            app = QApplication(sys.argv)
        self.app = app

        # AFTER the QApplication is created!
        from karabogui import icons
        icons.init()

    def process_qt_events(self):
        # Give the event loop 10ms to process its events
        self.app.processEvents(QEventLoop.AllEvents, 10)


@contextlib.contextmanager
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


@contextlib.contextmanager
def flushed_registry():
    """Avoid polluting the global binding controller registry with
    test controller classes
    """
    from karabogui.controllers import registry as registrymod

    controller_registry = registrymod._controller_registry.copy()
    registrymod._controller_registry.clear()
    model_registry = registrymod._controller_models.copy()
    registrymod._controller_models.clear()

    yield

    registrymod._controller_registry = controller_registry
    registrymod._controller_models = model_registry


def get_property_proxy(schema, name, device_id='TestDevice'):
    """Given a device schema and a property name, return a complete
    PropertyProxy object with a `root_proxy` of type `DeviceProxy`.
    """
    binding = build_binding(schema)
    root_proxy = DeviceProxy(binding=binding, device_id=device_id)
    root_proxy.status = DeviceStatus.SCHEMA
    return PropertyProxy(root_proxy=root_proxy, path=name)


def get_class_property_proxy(schema, name):
    """Given a device schema and a property name, return a complete
    PropertyProxy object with a `root_proxy` of type `DeviceClassProxy`.
    """
    binding = build_binding(schema)
    root = DeviceClassProxy(binding=binding, server_id='Test',
                            status=DeviceStatus.OFFLINE)
    return PropertyProxy(root_proxy=root, path=name)


@contextlib.contextmanager
def singletons(**objects):
    """Provide a collection of singletons to be used for the duration of a
    `with`-block.
    """
    # XXX: Yes, we're being naughty here. It's for testing though...
    singletons_dict = singletons_mod.__singletons
    # Remember what got replaced
    replaced = {k: singletons_dict[k] for k in objects if k in singletons_dict}
    try:
        # Replace and yield
        for key, obj in objects.items():
            singletons_dict[key] = obj
        yield
    finally:
        # Put things back as they were
        for key, obj in replaced.items():
            singletons_dict[key] = obj
        for key in objects:
            if key not in replaced:
                del singletons_dict[key]


def system_hash():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h['server.swerver'] = None
    h['server.swerver', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.OBSERVER,
        'deviceClasses': ['FooClass', 'BarClass'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.OBSERVER]
    }
    h['device.divvy'] = None
    h['device.divvy', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.OBSERVER,
        'capabilities': Capabilities.PROVIDES_SCENES,
        'serverId': 'swerver',
        'classId': 'FooClass',
        'status': 'online'
    }
    h['macro.macdonald'] = None
    h['macro.macdonald', ...] = {
        'host': 'BIG_IRON',
        'visibility': AccessLevel.OBSERVER,
        'serverId': 'swerver',
        'classId': 'BarClass',
        'status': 'incompatible'
    }

    h['device.orphan'] = None
    h['device.orphan', ...] = {
        'visibility': AccessLevel.OBSERVER,
        'serverId': '__none__',
        'classId': 'Parentless',
        'status': 'noserver'
    }

    return h


def system_hash_server_and_plugins():
    """Generate a system hash which will be built into a system tree
    """
    h = Hash()

    h['server.myserver'] = None
    h['server.myserver', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.OBSERVER,
        'deviceClasses': ['FooClass', 'BarClass'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.OBSERVER,
                         AccessLevel.OBSERVER]
    }
    h['server.samedeviceclasses'] = None
    h['server.samedeviceclasses', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.EXPERT,
        'deviceClasses': ['FooClass', 'BlahClass', 'HooClass_0'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.OBSERVER,
                         AccessLevel.OBSERVER]
    }

    h['server.differentaccesslevel'] = None
    h['server.differentaccesslevel', ...] = {
        'host': 'exflpxc_something',
        'visibility': AccessLevel.EXPERT,
        'deviceClasses': ['FooClass', 'BarClass', 'HooClass_1'],
        'visibilities': [AccessLevel.OBSERVER, AccessLevel.EXPERT,
                         AccessLevel.OBSERVER]
    }

    return h
